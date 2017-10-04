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

/* "@(#)85        1.15.1.19  src/htx/usr/lpp/htx/lib/htxsyscfg64/htxsyscfg.c, htx_libhtxsyscfg64, htxfedora 5/19/15 01:28:11";*/

#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>

#include "htxsyscfg64.h"
#include "hxiipc.h"

#define sem_id       (misc_htx_data->sem_id)
#define p_shm_hdr    (misc_htx_data->p_shm_hdr)

/* Global variables */
GLOBAL_SYSCFG *global_ptr = NULL;
int shm_id_htxsyscfg=-1;
extern char                        msg[];

int numberArray[MAX_CORES_PER_CHIP][3],numberArray_copy[MAX_CORES_PER_CHIP][3];
int array_index;
int 				syscfg_base_pg_idx;
unsigned int 		syscfg_base_page_size;
extern struct htx_data             *misc_htx_data;

/* readind env variables to avoid hardcodings of paths */

/* to read the entries for core exclusion from /tmp/syscfg_excluded_cores.txt file */
int read_core_exclusion_array(void)
{
	int i,j,k,l,flag;
	i = j = k = l = flag = 0;
	char buffer[20];
	char * pch;
	FILE *myFile;
	char fname[100];
	char tmp_path[64];
	char *tmp_path_temp;

	tmp_path_temp = getenv("HTX_LOG_DIR");
	if (tmp_path_temp == NULL){
		strcpy(tmp_path, "/tmp/");
    }
	else{
		strcpy(tmp_path,getenv("HTX_LOG_DIR"));
	}

	sprintf(fname,"%ssyscfg_excluded_cores.txt",tmp_path);
	myFile = fopen(fname, "r");
	//read file into array
	for(k=0; k<MAX_CORES_PER_CHIP; k++){
		for(l=0;l<3;l++){
			numberArray_copy[k][l] = -2;
		}
	}

    if (myFile == NULL)
		return 0;
	else{
		while(fgets(buffer, sizeof(buffer), myFile)){
			if((strcmp("\n",buffer))){
				pch = strtok (buffer,",");
				while (pch != NULL)
				{
					if(strcmp("node",pch)==0){
						flag = 1;
					}
					numberArray_copy[i][j]=atoi(pch);
					pch = strtok (NULL, ",");
					j++;
				}
				j = 0;
				if(flag == 1){
					i = 0;
					flag = 0;
				}
				else
					i++;
			}
		}
		fclose(myFile);
		return 0;
	}
}

/* For shared memory initialisation */

int init_syscfg(void)
{
	int r = 0;
	shm_id_htxsyscfg=shmget(SYSCFG_SHM_KEY,(sizeof(GLOBAL_SYSCFG)),(IPC_CREAT | S_IRWXU| S_IRWXG | S_IRWXO));
    if (shm_id_htxsyscfg == -1){
		sprintf(msg,"shm id creation failed with errno=%d \n", errno);
		hxfmsg(misc_htx_data, 0, HTX_HE_INFO, msg);
		return(101);
    }
    global_ptr=(GLOBAL_SYSCFG *)shmat(shm_id_htxsyscfg,0,0);
	if(global_ptr == (void *)(-1)) {
		sprintf(msg,"shmat failed with errno=%d \n", errno);
        hxfmsg(misc_htx_data, 0, HTX_HE_INFO, msg);
		return(102);
	}
	memset(global_ptr,0,(sizeof(GLOBAL_SYSCFG))) ;
	init_rwlocks();

    if ( sys_mount_check() ) {
        return (106);
    }


    r = update_syscfg();
    if ( r ) {
		sprintf(msg,"update_syscfg() failed with rc=%d\n", r);
        hxfmsg(misc_htx_data, 0, HTX_HE_INFO, msg);
		return(103);
    }
	return 0;
}

int init_syscfg_with_malloc(void){
    int rc = 0;
	if(global_ptr == NULL) {
		global_ptr=(GLOBAL_SYSCFG *)malloc(sizeof(GLOBAL_SYSCFG));
	    if(global_ptr == NULL) {
		    sprintf(msg,"shmat failed with errno=%d \n", errno);
			hxfmsg(misc_htx_data, 0, HTX_HE_INFO, msg);
			return(-1);
		}
	}

    memset(global_ptr,0,(sizeof(GLOBAL_SYSCFG))) ;
    init_rwlocks();

    if ( sys_mount_check() ) {
        return (105);
    }

    rc = update_syscfg();
    if ( rc ) {
		sprintf(msg,"update_syscfg() failed with rc=%d\n", rc);
		hxfmsg(misc_htx_data, 0, HTX_HE_INFO, msg);
		return(104);
    }
	return 0;
}

/* Process Shared Read/Write lock */

int init_rwlocks(void){
    int lockinit1,lockinit2,lockinit3,lockinit4,lockinit5,lockinit6,lockinit1_1;
    lockinit1=lockinit2=lockinit3=lockinit4=lockinit5=lockinit6=lockinit1_1=0;

    lockinit1 = pthread_rwlockattr_init(&rwlattr);
	if (lockinit1!=0  ) {
        if ((lockinit1 == EAGAIN) || (lockinit1 == ENOMEM)){
            usleep(10);
            lockinit1_1=pthread_rwlockattr_init(&rwlattr);
            if(lockinit1_1 !=0){
                sprintf(msg,"rwlock attr initialisation failed with rc=%d\n", lockinit1);
                hxfmsg(misc_htx_data, -1, HTX_HE_SOFT_ERROR, msg);
            }
        }
        else{
		sprintf(msg,"rwlock attr initialisation failed with rc=%d\n", lockinit1);
                hxfmsg(misc_htx_data, -1, HTX_HE_SOFT_ERROR, msg);
        }
    }

    lockinit1 = pthread_rwlockattr_setpshared(&rwlattr, PTHREAD_PROCESS_SHARED);
    if (lockinit1!=0  ) {
	sprintf(msg,"sharing rwlock attr across processes failed with rc=%d\n", lockinit1);
	hxfmsg(misc_htx_data, 0, HTX_HE_INFO, msg);
	}
	lockinit1=pthread_rwlock_init(&(global_ptr->global_lpar.rw),&rwlattr);
    if (lockinit1!=0  ) {
        if ( lockinit1 == EBUSY ){
		sprintf(msg,"global_ptr->global_lpar.rw_lock_init()1 failed with rc=%d\n", lockinit1);
                hxfmsg(misc_htx_data, 0, HTX_HE_INFO, msg);
        }
        else if ((lockinit1 == EAGAIN) || (lockinit1 == ENOMEM)){
            usleep(10);
		lockinit1_1=pthread_rwlock_init(&(global_ptr->global_lpar.rw),&rwlattr);
            if(lockinit1_1 !=0){
                    sprintf(msg,"global_ptr->global_lpar.rw_lock() failed in retry with rc=%d\n", lockinit1_1);
                    hxfmsg(misc_htx_data, -1, HTX_HE_SOFT_ERROR, msg);
			}
		}
        else{
                sprintf(msg,"global_ptr->global_lpar.rw_lock() failed with rc=%d\n", lockinit1);
                hxfmsg(misc_htx_data, -1, HTX_HE_SOFT_ERROR, msg);
        }
    }

	lockinit2=pthread_rwlock_init(&(global_ptr->global_memory.rw),&rwlattr);
    if (lockinit2!=0  ) {
        if ( lockinit2 == EBUSY ){
                sprintf(msg,"global_ptr->global_memory.rw_lock_init() failed with rc=%d\n", lockinit2);
                hxfmsg(misc_htx_data, 0, HTX_HE_INFO, msg);
        }
        else if ((lockinit1 == EAGAIN) || (lockinit1 == ENOMEM)){
            usleep(10);
			lockinit1_1=pthread_rwlock_init(&(global_ptr->global_memory.rw),&rwlattr);
            if(lockinit1_1 !=0){
                    sprintf(msg,"global_ptr->global_memory.rw_lock_init() failed with rc=%d\n", lockinit2);
                    hxfmsg(misc_htx_data, -1, HTX_HE_SOFT_ERROR, msg);
			}
		}
        else{
                sprintf(msg,"global_ptr->global_memory.rw_lock_init() failed with rc=%d\n", lockinit2);
                hxfmsg(misc_htx_data, -1, HTX_HE_SOFT_ERROR, msg);
        }
    }

	lockinit3=pthread_rwlock_init(&(global_ptr->global_cache.rw),&rwlattr);

    if (lockinit3!=0  ) {
        if ( lockinit3 == EBUSY ){
                sprintf(msg,"global_ptr->global_cache.rw_lock_init() failed with rc=%d\n", lockinit3);
                hxfmsg(misc_htx_data, 0, HTX_HE_INFO, msg);
        }
        else if ((lockinit1 == EAGAIN) || (lockinit1 == ENOMEM)){
            usleep(10);
			lockinit1_1=pthread_rwlock_init(&(global_ptr->global_cache.rw),&rwlattr);
            if(lockinit1_1 !=0){
                    sprintf(msg,"global_ptr->global_cache.rw_lock_init() failed with rc=%d\n", lockinit3);
                    hxfmsg(misc_htx_data, -1, HTX_HE_SOFT_ERROR, msg);
			}
		}
        else{
                sprintf(msg,"global_ptr->global_cache.rw_lock_init() failed with rc=%d\n", lockinit3);
                hxfmsg(misc_htx_data, -1, HTX_HE_SOFT_ERROR, msg);
        }
    }

	lockinit4=pthread_rwlock_init(&(global_ptr->syscfg.rw),&rwlattr);
    if (lockinit4!=0  ) {
        if ( lockinit4 == EBUSY ){
                sprintf(msg,"global_ptr->syscfg.rw_lock_init() failed with rc=%d\n", lockinit4);
                hxfmsg(misc_htx_data, 0, HTX_HE_INFO, msg);
        }
        else if ((lockinit1 == EAGAIN) || (lockinit1 == ENOMEM)){
            usleep(10);
			lockinit1_1=pthread_rwlock_init(&(global_ptr->syscfg.rw),&rwlattr);
            if(lockinit1_1 !=0){
                    sprintf(msg,"global_ptr->syscfg.rw_lock_init() failed with rc=%d\n", lockinit4);
                    hxfmsg(misc_htx_data, -1, HTX_HE_SOFT_ERROR, msg);
			}
		}
        else{
                sprintf(msg,"global_ptr->syscfg.rw_lock_init() failed with rc=%d\n", lockinit4);
                hxfmsg(misc_htx_data, -1, HTX_HE_SOFT_ERROR, msg);
        }
    }

	lockinit5=pthread_rwlock_init(&(global_ptr->system_cpu_map.stat.rw),&rwlattr);
    if (lockinit5!=0  ) {
			sprintf(msg,"global_ptr->system_cpu_map.stat.rw_lock_init() failed with rc=%d\n", lockinit5);
			hxfmsg(misc_htx_data, 0, HTX_HE_INFO, msg);
    }
    if (lockinit5!=0  ) {
        if ( lockinit5 == EBUSY ){
                sprintf(msg,"global_ptr->system_cpu_map.stat.rw_lock_init() failed with rc=%d\n", lockinit5);
                hxfmsg(misc_htx_data, 0, HTX_HE_INFO, msg);
        }
        else if ((lockinit1 == EAGAIN) || (lockinit1 == ENOMEM)){
            usleep(10);
			lockinit1_1=pthread_rwlock_init(&(global_ptr->system_cpu_map.stat.rw),&rwlattr);
            if(lockinit1_1 !=0){
                    sprintf(msg,"global_ptr->system_cpu_map.stat.rw_lock_init() failed with rc=%d\n", lockinit5);
                    hxfmsg(misc_htx_data, -1, HTX_HE_SOFT_ERROR, msg);
			}
		}
        else{
                sprintf(msg,"global_ptr->system_cpu_map.stat.rw_lock_init() failed with rc=%d\n", lockinit5);
                hxfmsg(misc_htx_data, -1, HTX_HE_SOFT_ERROR, msg);
        }
    }

	lockinit6=pthread_rwlock_init(&(global_ptr->global_core.rw),&rwlattr);
    if (lockinit6!=0  ) {
        if ( lockinit6 == EBUSY ){
                sprintf(msg,"global_ptr->global_core.rw_lock_init() failed with rc=%d\n", lockinit6);
                hxfmsg(misc_htx_data, 0, HTX_HE_INFO, msg);
        }
        else if ((lockinit1 == EAGAIN) || (lockinit1 == ENOMEM)){
            usleep(10);
			lockinit1_1=pthread_rwlock_init(&(global_ptr->global_core.rw),&rwlattr);
            if(lockinit1_1 !=0){
                    sprintf(msg,"global_ptr->global_core.rw_lock_init() failed with rc=%d\n", lockinit6);
                    hxfmsg(misc_htx_data, -1, HTX_HE_SOFT_ERROR, msg);
			}
		}
        else{
                sprintf(msg,"global_ptr->global_core.rw_lock_init() failed with rc=%d\n", lockinit6);
                hxfmsg(misc_htx_data, -1, HTX_HE_SOFT_ERROR, msg);
        }
    }
	return 0;
}

int attach_to_syscfg(void)
{
    shm_id_htxsyscfg=shmget(SYSCFG_SHM_KEY,(sizeof(GLOBAL_SYSCFG)),(S_IRWXU| S_IRWXG | S_IRWXO));
    if (shm_id_htxsyscfg == -1)/* perror ("Creation ");*/{
            sprintf(msg,"inside attach_to_syscfg, shm_id creation failed= %d \n",errno);
            hxfmsg(misc_htx_data, 0, HTX_HE_SOFT_ERROR, msg);
		return(107);
    }

	global_ptr=(GLOBAL_SYSCFG *)shmat(shm_id_htxsyscfg,0,0);
	if(global_ptr == (void *)(-1)) {
			sprintf(msg,"shmat failed inside attach_to_syscfg with errno=%d", errno);
			hxfmsg(misc_htx_data, 0, HTX_HE_SOFT_ERROR, msg);
		return(108);
	}
	return 0;
}


/* shared memory deletion */
int detach_syscfg(void)
{
	int i,shmdt_check,temp_shm_key;
	int lock1,lock2,lock3,lock4,lock5,lock6;
	i=shmdt_check = temp_shm_key = 0;
	lock1=lock2=lock3=lock4=lock5=lock6=0;

    temp_shm_key = shmget(SYSCFG_SHM_KEY,(sizeof(GLOBAL_SYSCFG)),(IPC_CREAT | IPC_EXCL | S_IRWXU| S_IRWXG | S_IRWXO));
    if ( temp_shm_key == -1 && errno == EEXIST ) {
	    lock1=pthread_rwlock_destroy(&(global_ptr->global_lpar.rw));
		if (lock1!=0  ) {
				sprintf(msg,"global_ptr->global_lpar.rw_lock_destroy() failed with rc=%d\n", lock1);
	            hxfmsg(misc_htx_data, -1, HTX_HE_SOFT_ERROR, msg);
		}
	    lock2=pthread_rwlock_destroy(&(global_ptr->global_memory.rw));
		if (lock2!=0  ) {
				sprintf(msg,"global_ptr->global_memory.rw_lock_destroy() failed with rc=%d\n", lock2);
	            hxfmsg(misc_htx_data, -1, HTX_HE_SOFT_ERROR, msg);
		}
	    lock3=pthread_rwlock_destroy(&(global_ptr->global_cache.rw));
		if (lock3!=0  ) {
				sprintf(msg,"global_ptr->global_cache.rw_lock_destroy() failed with rc=%d\n", lock3);
	            hxfmsg(misc_htx_data, -1, HTX_HE_SOFT_ERROR, msg);
		}
	    lock4=pthread_rwlock_destroy(&(global_ptr->syscfg.rw));
		if (lock4!=0  ) {
				sprintf(msg,"global_ptr->syscfg.rw_lock_destroy() failed with rc=%d\n", lock4);
	            hxfmsg(misc_htx_data, -1, HTX_HE_SOFT_ERROR, msg);
		}
	    lock5=pthread_rwlock_destroy(&(global_ptr->system_cpu_map.stat.rw));
		if (lock5!=0  ) {
				sprintf(msg,"global_ptr->system_cpu_map.stat.rw_lock_destroy() failed with rc=%d\n", lock5);
	            hxfmsg(misc_htx_data, -1, HTX_HE_SOFT_ERROR, msg);
		}
	    lock6=pthread_rwlock_destroy(&(global_ptr->global_core.rw));
		if (lock6!=0  ) {
				sprintf(msg,"global_ptr->global_core.rw_lock_destroy() failed with rc=%d\n", lock6);
	            hxfmsg(misc_htx_data, -1, HTX_HE_SOFT_ERROR, msg);
		}

	    shmdt_check = shmdt(global_ptr);
		if (shmdt_check ==  -1){
				sprintf(msg,"shmdt failed with errno=%d", errno);
				hxfmsg(misc_htx_data, 0, HTX_HE_INFO, msg);
		}
	    i=shmctl(shm_id_htxsyscfg,IPC_RMID,NULL);
		if(i==-1){
				sprintf(msg,"inside detach_syscfg, shmctl failed= %d ",errno);
				hxfmsg(misc_htx_data, 0, HTX_HE_INFO, msg);
		}
		return(shmdt_check | i);
    }
	else{
            sprintf(msg,"shm not existing ! \n");
            hxfmsg(misc_htx_data, 0, HTX_HE_INFO, msg);
		return(-1);
	}
}

int detach_syscfg_with_malloc(void){
	if(global_ptr != NULL){
		free(global_ptr);
		global_ptr = NULL;
	}
	return 0;
}

/* APIs to retrieve specific details */

/*Returns output of a shell command (cmd) in a string*/
int get_cmd_op(char *dest,const char *cmd)
{
    FILE *fp;
    int i=0,count=0;

	/*while(count < 5){*/
	    fp=popen(cmd,"r");
	    if ( fp != NULL ) {
    	    while( !feof(fp) && !ferror(fp)) {
        	    char ch;
        	    ch = fgetc(fp);
				if ( (int)ch < 32 || (int)ch > 127 ) {
            	      ch = ' ';
            	 }
           		 dest[i++] = ch;
        	}
    	}
    	else {
                    sprintf(msg,"syscfg %d:Failed popen for command %s ,errno=%di,re-try count=%d\n",__LINE__,cmd,errno,count);
                    hxfmsg(misc_htx_data, 0, HTX_HE_SOFT_ERROR, msg);
                count++;

    	}
   /* }*/
    if(count == 5){
        pclose(fp);
        return -1;
	}
    pclose(fp);
    dest[i]='\0';
    return(0);
}

#ifdef __HTX_LINUX__
int sys_mount_check(void)
{
    int sys_mounted;
    char sys_avail[100];

    get_cmd_op(sys_avail, "mount | grep sys | wc -l");
    sys_mounted = atoi(sys_avail);

    if ( sys_mounted == 0 ) {
        if (mkdir("/sys", 0777)) {
				sprintf(msg,"syscfg:/sys mkdir failed");
				hxfmsg(misc_htx_data, 0, HTX_HE_SOFT_ERROR, msg);
            return -1;
        }

        if (mount("sys", "/sys", "sysfs", MS_NOATIME, NULL)){
				sprintf(msg,"syscfg:/sys mkdir failed in the second try");
			hxfmsg(misc_htx_data, 0, HTX_HE_SOFT_ERROR, msg);
            return -1;
        }
    }

    return 0;
}
#endif

/*Removes new line at end, if present.*/
void remove_newline_at_end(char *dest)
{
    int len;
    len=strlen(dest);

    if(dest[len-1]=='\n') {
        dest[len-1]='\0';
    }
}

/*Retrieves hostname*/
int get_hostname_update(void)
{
	int rc = 0;
	gethostname((global_ptr->global_lpar.host_name),100);
    remove_newline_at_end(global_ptr->global_lpar.host_name);
	return(rc);
}

void get_hostname(char* dest)
{
   strcpy(dest,(global_ptr->global_lpar.host_name));
}

/*Retrieves MTM*/
int get_mtm(void)
{
    int rc;
    rc = get_cmd_op((global_ptr->global_lpar.mtm),"cat /proc/device-tree/model ");
    remove_newline_at_end(global_ptr->global_lpar.mtm);
    return(rc);
}

/*Retrieves OS version*/
int get_osversion(void)
{
    int rc;
    rc = get_cmd_op((global_ptr->global_lpar.os_version),"cat /proc/sys/kernel/osrelease ");
    remove_newline_at_end(global_ptr->global_lpar.os_version);
    return(rc);
}

/*Retrieves details of physical, virtual and logical processors*/

int phy_logical_virt_cpus_update(void)
{
    int r1;
    char dest[100];
	htxsyscfg_cpus_t *t;
    t =&(global_ptr->global_lpar.cpus);

    r1 = get_cmd_op(dest,"cat /proc/cpuinfo | grep 'processor' | wc -l");
    t->l_cpus = atoi(dest);         /*Logical cpus*/

    #ifdef BML
        t->p_cpus = 1;          /* On BML, partition entitle capacity is 1 */
        t->v_cpus = t->l_cpus;  /* On BML, v_cpus = l_cpus */
        t->phy_to_virt = (t->p_cpus *100)/ t->v_cpus; /*Physical-to-virtual cpu percentage*/

        t->phy_to_logical = (t->p_cpus *100)/ t->l_cpus; /*Physical-to-logical cpu percentage*/
        return r1;
    #else
	int r2, r3;
    r2 = get_cmd_op(dest,"ls -l /proc/device-tree/cpus | grep -i power | wc -l");
    t->p_cpus=atoi(dest);     /*Physical cpus*/

    r3 = get_cmd_op(dest,"ls -l /proc/device-tree/cpus | grep ^d | awk '($NF ~ /POWER/)' | wc -l");
    t->v_cpus=atoi(dest);

    t->phy_to_virt = (t->p_cpus *100)/ t->v_cpus; /*Physical-to-virtual cpu percentage*/

    t->phy_to_logical = (t->p_cpus *100)/ t->l_cpus; /*Physical-to-logical cpu percentage*/

    return(r1 | r2 | r3);
    #endif
}

int phy_logical_virt_cpus(htxsyscfg_cpus_t *t)
{
	int rc1,rc2,rc1_1;
	rc1=rc2=rc1_1=0;
    rc1 = pthread_rwlock_rdlock(&(global_ptr->global_lpar.rw));
    if (rc1!=0  ) {
        if ( rc1 == EDEADLK ) {
                sprintf(msg,"lock inside phy_logical_virt_cpus failed with errno=%d\n",rc1);
                hxfmsg(misc_htx_data, -1, HTX_HE_HARD_ERROR, msg);
				return(rc1);
        }
        else if ( rc1 == EAGAIN ) {
			usleep(10);
			rc1_1 = pthread_rwlock_tryrdlock(&(global_ptr->global_lpar.rw));
			if(rc1_1!=0){
		            sprintf(msg,"global_ptr->global_lpar.rw_lock() failed in retry with rc=%d\n", rc1_1);
		            hxfmsg(misc_htx_data, -1, HTX_HE_HARD_ERROR, msg);
					return(rc1_1);
			}
        }
		else{
                sprintf(msg,"global_ptr->global_lpar.rw_lock() failed with rc=%d\n",rc1);
				hxfmsg(misc_htx_data, -1, HTX_HE_HARD_ERROR, msg);
				return(rc1);
		}
    }

	*t =(global_ptr->global_lpar.cpus);
	rc2=pthread_rwlock_unlock(&(global_ptr->global_lpar.rw));
	if (rc2 !=0  ) {
			sprintf(msg,"unlock inside phy_logical_virt_cpus failed with errno=%d\n",rc2);
			hxfmsg(misc_htx_data, -1, HTX_HE_HARD_ERROR, msg);
			return(rc2);
	}
    return 0;
}

/*Retrieves lpar level details*/
int get_lpar_details_update(void)
{
    int r1,r2,r3,r4,r5;
    htxsyscfg_lpar_t *t;
    t=&(global_ptr->global_lpar);

    r5 = get_hostname_update();      /* Host name */

    r1 = get_mtm();                 /* Machine type model */

    r2 = get_osversion();    /* Os version of the system */

#ifdef __AWAN__
	strcpy(t->com_ip, "127.0.0.1");
	r3 = 0;
#else
    r3 = get_cmd_op(t->com_ip, "ifconfig | grep 'inet addr:' | grep -v '127.0.0.1' | cut -d: -f2 | awk '{print $1}' | head -1");    /*Host ip*/
    remove_newline_at_end(t->com_ip);
#endif
    r4 = get_cmd_op(t->time_stamp,"date");       /*Date and time*/
    remove_newline_at_end(t->time_stamp);

    /*r5 = phy_logical_virt_cpus(&(t->cpus));*/ /*Physical,logical and virtual cpus*/

    return ( r1 | r2 | r3 | r4 | r5 );
}

int get_lpar_details(htxsyscfg_lpar_t *t)
{
    int rc1,rc2,rc1_1;
	rc1=rc2=0;

    rc1 = pthread_rwlock_rdlock(&(global_ptr->global_lpar.rw));
    if (rc1!=0  ) {
        if ( rc1 == EDEADLK ) {
                sprintf(msg,"lock inside get_lpar_details failed with errno=%d\n",rc1);
                hxfmsg(misc_htx_data, -1, HTX_HE_HARD_ERROR, msg);
				return(rc1);
        }
        else if ( rc1 == EAGAIN ) {
			usleep(10);
			rc1_1 = pthread_rwlock_tryrdlock(&(global_ptr->global_lpar.rw));
			if(rc1_1!=0){
		            sprintf(msg,"global_ptr->global_lpar.rw_lock() in get_lpar_details failed in retry with rc=%d\n", rc1_1);
		            hxfmsg(misc_htx_data, -1, HTX_HE_HARD_ERROR, msg);
					return(rc1_1);
			}
        }
		else{
                sprintf(msg,"global_ptr->global_lpar.rw_lock()in get_lpar_details  failed with rc=%d\n",rc1);
				hxfmsg(misc_htx_data, -1, HTX_HE_HARD_ERROR, msg);
				return(rc1);
		}
    }
	*t=(global_ptr->global_lpar);
	rc2 = pthread_rwlock_unlock(&(global_ptr->global_lpar.rw));
	if (rc2 !=0  ) {
			sprintf(msg,"unlock inside get_lpar_details failed with errno=%d\n",rc1);
			hxfmsg(misc_htx_data, -1, HTX_HE_HARD_ERROR, msg);
			return(rc2);
	}
    return 0;
}

/*Retrieves smt details*/
int get_smt_details_update(void)
{
    char dest[100];
	htxsyscfg_smt_t *t;
    t =&(global_ptr->global_core.smtdetails);

    if ( sys_mount_check() ) {
        return (-1);
    }
    t->smt_threads     = global_ptr->global_core.smtdetails.smt_threads;
    t->min_smt_threads = global_ptr->global_core.smtdetails.min_smt_threads;/*min and max smt is because in case of hotplug supported env there is no fix cpu numbers per core*/
    t->max_smt_threads = global_ptr->global_core.smtdetails.max_smt_threads;

    return 0;
}

int get_smt_details(htxsyscfg_smt_t *t)
{
	int rc1,rc2,rc1_1;
	rc1=rc2=0;
	rc1 = pthread_rwlock_rdlock(&(global_ptr->global_core.rw));
    if (rc1!=0  ) {
        if ( rc1 == EDEADLK ) {
                sprintf(msg,"lock inside get_smt_details failed with errno=%d\n",rc1);
                hxfmsg(misc_htx_data, -1, HTX_HE_HARD_ERROR, msg);
				return(rc1);
        }
        else if ( rc1 == EAGAIN ) {
			usleep(10);
			rc1_1 = pthread_rwlock_tryrdlock(&(global_ptr->global_core.rw));
			if(rc1_1!=0){
		            sprintf(msg,"lock inside get_smt_details failed in retry with errno=%d\n",rc1);
		            hxfmsg(misc_htx_data, -1, HTX_HE_HARD_ERROR, msg);
					return(rc1_1);
			}
        }
		else{
                sprintf(msg,"lock inside get_smt_details failed with errno=%d\n",rc1);
				hxfmsg(misc_htx_data, -1, HTX_HE_HARD_ERROR, msg);
				return(rc1);
		}
    }
    *t =(global_ptr->global_core.smtdetails);
	rc2 = pthread_rwlock_unlock(&(global_ptr->global_core.rw));
	if (rc2 !=0  ) {
			sprintf(msg,"unlock inside get_smt_details failed with errno=%d\n",rc2);
			hxfmsg(misc_htx_data, -1, HTX_HE_HARD_ERROR, msg);
			return(rc2);
	}
    return 0;
}

/*Retrieves processor frequency in Hz*/
int get_min_smt(void)
{
	int min_smt,smt,i;
	min_smt = get_smt_status(0);
	for(i=1; i<global_ptr->system_cpu_map.stat.cores; i++){
		smt = get_smt_status(i);
		if(min_smt > smt)
		{
			min_smt = smt;
		}
	}
	global_ptr->global_core.smtdetails.min_smt_threads=min_smt;
	return 0;
}

int get_max_smt(void)
{
    int max_smt,smt,i;
    max_smt = get_smt_status(0);
    for(i=1; i<global_ptr->system_cpu_map.stat.cores; i++){
        smt = get_smt_status(i);
        if(max_smt < smt)
        {
            max_smt = smt;
        }
    }
	global_ptr->global_core.smtdetails.max_smt_threads=max_smt;
    return 0;
}

int get_hw_smt(void)
{
	unsigned int Pvr;
	int dd1_bit = 0;
	Pvr = global_ptr->global_pvr;

	if (Pvr == PV_POWER6){
		global_ptr->global_core.smtdetails.smt_threads = 2;
		return 0;
	}
	else if (Pvr == PV_POWER7 || Pvr == PV_POWER7PLUS){
		global_ptr->global_core.smtdetails.smt_threads=  4;
		return 0;
	}
	else if (Pvr == PV_POWER8_MURANO || Pvr == PV_POWER8_VENICE || Pvr == PV_POWER8_PRIME){
		global_ptr->global_core.smtdetails.smt_threads= 8;
		return 0;
	}
    else if (Pvr == PV_POWER9_NIMBUS || Pvr == PV_POWER9_CUMULUS){ 
		dd1_bit = get_p9_core_type();
		if (dd1_bit == DD1_NORMAL_CORE)
            global_ptr->global_core.smtdetails.smt_threads =  4; 
		else if (dd1_bit == DD1_FUSED_CORE)
			global_ptr->global_core.smtdetails.smt_threads =  8;
		return 0;
	}
	else {
            sprintf(msg,"syscfg:In get_hw_smt():Unknown Pvr %u\n",Pvr);
            hxfmsg(misc_htx_data, 0, HTX_HE_INFO, msg);
		global_ptr->global_core.smtdetails.smt_threads= -1;
		return -1;
	}
	return 0;
}

long long get_proc_freq(void)
{
char ch, freq_str[20];
    float freq=0;
    int i=0;
    FILE *fp;


    fp=popen("cat /proc/cpuinfo | grep -m 1 clock | cut -f 2 -d :","r");
    if ( fp != NULL ) {
        while(1) {
            ch=fgetc(fp);

            if(feof(fp)) {
                break;
            }
            if(ferror(fp)) {
                pclose(fp);
                return(-1);
            }

            if( isdigit(ch) || ch == '.' ) {
                freq_str[i++]=ch;
            }

            if ( isalpha(ch) ) {
                break;
            }
        }
    }
    else {
        pclose(fp);
			sprintf(msg,"syscfg:popen failed for /proc/cpuinfo\n");
		hxfmsg(misc_htx_data, 0, HTX_HE_INFO, msg);
        return (-1);
    }

    freq_str[i]='\0';

    freq = atof(freq_str);
    freq = freq * (1000*1000); /* Converting MHz to Hz. Considering /proc o/p in MHz */

    if ( ch == 'G' || ch == 'g' ) {
        freq = freq * 1000;
    }
    pclose(fp);

    return ((long long)freq);
}

/*Retrieves timebase details*/
long long get_timebase(void)
{
    float tb_freq;
    int i,num;
	i = num = tb_freq = 0;
    FILE *fp;
    char ch;

    fp=popen("cat /proc/cpuinfo | grep timebase |cut -f 2 -d :","r");
    if ( fp != NULL ) {
        while(1) {
            ch=fgetc(fp);

            if(feof(fp)) {
                break;
            }
            if(ferror(fp)) {
                pclose(fp);
                return(-1);
            }


            if(isdigit(ch)) {
                num=ch-'0';
                tb_freq=(tb_freq*10)+ num;
                i++;
            }
        }
    }
    else {
        pclose(fp);
		sprintf(msg,"syscfg:popen failed for /proc/cpuinfo\n");
		hxfmsg(misc_htx_data, 0, HTX_HE_INFO, msg);
        return (-1);
    }

    pclose(fp);

    return ((long long)tb_freq);
}


/*Retrieves core details*/
int get_core_details_update(void)
{
    int rc=0;
    htxsyscfg_core_t *t;
    t =&(global_ptr->global_core);
    (t->smtdetails)=(global_ptr->global_core.smtdetails);
    t->cpu_clock_freq = get_proc_freq();   /*Processor frequency*/
    #ifdef __HTX_LINUX__
    t->tb_clock_freq = get_timebase();
    #endif
    return(rc);
}

int get_core_details(htxsyscfg_core_t *t)
{
	int rc1,rc2,rc1_1;
	rc1=rc2=0;
	rc1 = pthread_rwlock_rdlock(&(global_ptr->global_core.rw));
    if (rc1!=0  ) {
        if ( rc1 == EDEADLK ) {
                sprintf(msg,"lock inside get_core_details failed with errno=%d\n",rc1);
                hxfmsg(misc_htx_data, -1, HTX_HE_HARD_ERROR, msg);
				return(rc1);
        }
        else if ( rc1 == EAGAIN ) {
			usleep(10);
			rc1_1 = pthread_rwlock_tryrdlock(&(global_ptr->global_core.rw));
			if(rc1_1!=0){
		            sprintf(msg,"lock inside get_core_details failed in retry with errno=%d\n",rc1_1);
		            hxfmsg(misc_htx_data, -1, HTX_HE_HARD_ERROR, msg);
					return(rc1_1);
			}
        }
		else{
                sprintf(msg,"lock inside get_core_details failed with errno=%d\n",rc1);
				hxfmsg(misc_htx_data, -1, HTX_HE_HARD_ERROR, msg);
				return(rc1);
		}
    }
	*t = (global_ptr->global_core);

	rc2 = pthread_rwlock_unlock(&(global_ptr->global_core.rw));
	if (rc2 !=0  ) {
			sprintf(msg,"unlock inside get_core_details failed with errno=%d\n",rc2);
			hxfmsg(misc_htx_data, -1, HTX_HE_HARD_ERROR, msg);
			return(rc2);
	}
    return 0;
}

/*Retrieves memory size*/
int get_memory_size_update(void)
{
    htxsyscfg_memsize_t *t;
    t=&(global_ptr->global_memory.mem_size);

    #ifdef __HTX_LINUX__
	char dest[100];
    int r1, r2;

    r1 = get_cmd_op(dest, "cat /proc/meminfo | grep MemTotal | awk '{ print $2 }'");
    t->real_mem = atoll(dest);
    t->real_mem = (t->real_mem * 1024);/*Size of total real memory in bytes*/

    r2 = get_cmd_op(dest, "cat /proc/meminfo | grep MemFree | awk '{ print $2 }'");
    t->free_real_mem = atoll(dest);
    t->free_real_mem = (t->free_real_mem * 1024);/*Size of free real memory in bytes*/

    return ( r1 | r2 );
 #else
    int rc;
    perfstat_memory_total_t mem;
    rc = perfstat_memory_total(NULL, &mem, sizeof(perfstat_memory_total_t),1);

    t->real_mem = mem.real_total;     /*size of real memory in bytes*/
    t->virt_mem = mem.virt_total;     /*Size of virtual memory in bytes*/
    t->free_real_mem = mem.real_free; /*Size of free real memory in bytes*/

    return(rc);
    #endif
}

int get_memory_size(htxsyscfg_memsize_t *t)
{
	int rc1,rc2,rc1_1;
	rc1=rc2=rc1_1=0;
	rc1 = pthread_rwlock_rdlock(&(global_ptr->global_memory.rw));
    if (rc1!=0  ) {
        if ( rc1 == EDEADLK ) {
                sprintf(msg,"lock inside get_memory_size failed with errno=%d\n",rc1);
                hxfmsg(misc_htx_data, -1, HTX_HE_HARD_ERROR, msg);
				return(rc1);
        }
        else if ( rc1 == EAGAIN ) {
			usleep(10);
			rc1_1 = pthread_rwlock_tryrdlock(&(global_ptr->global_memory.rw));
			if(rc1_1!=0){
		            sprintf(msg,"lock inside get_memory_size failed in retry with errno=%d\n",rc1_1);
		            hxfmsg(misc_htx_data, -1, HTX_HE_HARD_ERROR, msg);
					return(rc1_1);
			}
        }
		else{
                sprintf(msg,"lock inside get_memory_size failed with errno=%d\n",rc1);
				hxfmsg(misc_htx_data, -1, HTX_HE_HARD_ERROR, msg);
				return(rc1);
		}
    }
	*t=(global_ptr->global_memory.mem_size);

	rc2 = pthread_rwlock_unlock(&(global_ptr->global_memory.rw));
	if (rc2 !=0  ) {
			sprintf(msg,"unlock inside get_memory_size failed with errno=%d\n",rc2);
			hxfmsg(misc_htx_data, -1, HTX_HE_HARD_ERROR, msg);
			return(rc2);
	}
    return 0;
}

/********************************
* Retrieves memory pools details
*-On success returns "total number of pools"
*-On failure,returns -4
********************************/
int get_memory_pools(void) {

	htxsyscfg_mempools_t *t;
	FILE *fp=0;
	char command[200],fname[100];
	int i,j,rc,cpu,lcpu,huge_page_size;
	int tot_pools = -1;
	unsigned long total_pages, free_pages;
	total_pages = free_pages = 0;

	char tmp_path[64],scripts_path[64];
	char *tmp_path_temp;

	tmp_path_temp = getenv("HTX_LOG_DIR");
	if (tmp_path_temp == NULL){
		strcpy(tmp_path, "/tmp/");
    }
	else{
		strcpy(tmp_path,getenv("HTX_LOG_DIR"));
	}

	tmp_path_temp = getenv("HTXSCRIPTS");
	if (tmp_path_temp == NULL){
		strcpy(scripts_path, "/usr/lpp/htx/etc/scripts/");
    }
	else{
		strcpy(scripts_path,getenv("HTXSCRIPTS"));
	}


	 t =&(global_ptr->global_memory.mem_pools)[0];


	/* Initialize all page sizes to unsupported */

	for(i=0;i<MAX_POOLS;i++){
    	for (j=0; j<MAX_PAGE_SIZES;j++){
        	t[i].page_info_per_mempool[j].supported = FALSE;
    	}
	}
	sprintf(fname,"%s/mem_pool_details",tmp_path);
	sprintf(command,"%s/get_mem_pool_details.sh > %s",scripts_path,fname);
	errno = 0;
	rc = system(command);
    if (rc == -1){
            sprintf(msg,"%sget_mem_pool_details.sh failed with errno = %d \n",scripts_path,errno);
            hxfmsg(misc_htx_data, 0, HTX_HE_INFO, msg); 
            /*return -4;*/ /* workaround for the race condition with the sigchld */
    }
	
	if ((fp=fopen(fname,"r"))==NULL){
		sprintf(msg,"fopen of file %s failed with errno=%d",fname,errno);
		hxfmsg(misc_htx_data, -1, HTX_HE_SOFT_ERROR, msg);
		return -4;
	}
	rc = fscanf(fp,"num_nodes= %d\n",&tot_pools);
	if (rc == 0 || rc == EOF) {
		sprintf(msg,"fscanf of num_nodes on file %s failed with errno =%d",fname,errno);
		hxfmsg(misc_htx_data, -1, HTX_HE_SOFT_ERROR, msg);
		fclose(fp);
		return -4;
	}
	for(i=0;(i<tot_pools) && (i<MAX_POOLS);i++) {
		t[i].has_cpu_or_mem = FALSE;
		rc = fscanf(fp,"node_num=%d,mem_avail=%llu,mem_free=%llu,cpus_in_node=%d,Hugepagesize=%d,HugePages_Total=%lu,HugePages_Free=%lu,cpus,",&t[i].node_num,&t[i].mem_total,&t[i].mem_free,
					&t[i].num_procs,&huge_page_size,&total_pages,&free_pages);
		if (rc == 0 || rc == EOF) {
			sprintf(msg,"fscanf: fetching mem and cpu details in node =%d from file %s failed with rc = %d, errno =%d",i,fname,rc, errno);
			hxfmsg(misc_htx_data, -1, HTX_HE_SOFT_ERROR, msg);
			fclose(fp);
			return -4;
		}
		t[i].mem_total = t[i].mem_total * 1024;
		t[i].mem_free  = t[i].mem_free  * 1024;
		if(t[i].mem_free) {
			t[i].page_info_per_mempool[syscfg_base_pg_idx].page_size  = syscfg_base_page_size;
			t[i].page_info_per_mempool[syscfg_base_pg_idx].free_pages = (t[i].mem_free/syscfg_base_page_size);
			t[i].page_info_per_mempool[syscfg_base_pg_idx].total_pages = (t[i].mem_total/syscfg_base_page_size);
			t[i].page_info_per_mempool[syscfg_base_pg_idx].supported  = TRUE;
			if(t[i].num_procs) {
				t[i].has_cpu_or_mem = TRUE;
			}
		}
		if(huge_page_size == 16384) {
			t[i].page_info_per_mempool[PAGE_INDEX_16M].page_size  = 16777216;
			t[i].page_info_per_mempool[PAGE_INDEX_16M].supported  = TRUE;
			t[i].page_info_per_mempool[PAGE_INDEX_16M].total_pages = total_pages;
			t[i].page_info_per_mempool[PAGE_INDEX_16M].free_pages = free_pages;
		}
		else if(huge_page_size == 2048) {
			t[i].page_info_per_mempool[PAGE_INDEX_2M].page_size  = 2097152;
			t[i].page_info_per_mempool[PAGE_INDEX_2M].supported  = TRUE;
			t[i].page_info_per_mempool[PAGE_INDEX_2M].total_pages = total_pages;
			t[i].page_info_per_mempool[PAGE_INDEX_2M].free_pages = free_pages;
		}

	/*	printf("node_num=%d,mem_avail=%llu,mem_free=%llu,cpus_in_node=%d,Hugepagesize=%d,HugePages_Total=%d,HugePages_Free=%d,cpus=",t[i].node_num,t[i].mem_total,t[i].mem_free,
				t[i].num_procs,huge_page_size,t[i].page_info_per_mempool[PAGE_INDEX_16M].total_pages,t[i].page_info_per_mempool[PAGE_INDEX_16M].free_pages);
	*/
		for(cpu=0;cpu < t[i].num_procs;cpu++) {
			rc = fscanf(fp,":%d",&lcpu);
			if ( rc == 0 || rc == EOF) {
				sprintf(msg,"fscanf: cpu fetch error in node=%d from file %s with errno=%d and rc=%d\n",i,fname,errno,rc);
				hxfmsg(misc_htx_data, -1, HTX_HE_SOFT_ERROR, msg);
				fclose(fp);
				return -4;
			}
			t[i].procs_per_pool[cpu] = lcpu;
			/*printf("%d:",t[i].procs_per_pool[cpu]);*/
		}
		/*printf("\n");*/
		if( fscanf(fp,"\n")){};
	}
	fclose(fp);
    if(tot_pools > 0) {
		global_ptr->global_memory.num_numa_nodes = tot_pools;
    }
	else{
		sprintf(msg,"global_memory.num_numa_nodes giving value of %d \n",global_ptr->global_memory.num_numa_nodes);
		hxfmsg(misc_htx_data, -1, HTX_HE_SOFT_ERROR, msg);
	}
	return (tot_pools);
}

/********************************
* Retrieves page wise memory details
*-On success returns 0
*-On failure,returns -3
********************************/
int  get_page_details(void){
	htxsyscfg_pages_t *t;
    FILE *fp=0;
	int rc,i,huge_page_size;
                t =&(global_ptr->global_memory.page_details)[0];

    /*fp=popen("cat /proc/meminfo | grep SwapTotal | awk '{print $2}' ","r");
 *     if (fp == NULL || fp == -1 ) {
 *             return -1;
 *                 }
 *                     rc=fscanf(fp,"%lu\n",&mem_info.pspace_avail);
 *                         if (rc == 0 || rc == EOF) {
 *                                 return -1;
 *                                     }
 *                                         pclose(fp);
 *                                             */
    /* Initialize all page sizes to unsupported */
    for ( i=0; i<MAX_PAGE_SIZES;i++){
        t[i].supported = FALSE;
    }
    fp=popen("getconf PAGESIZE", "r");
    if (fp == NULL || fp == (void *)-1 ) {
        pclose(fp);
        return -3;
    }
	rc = fscanf(fp,"%d\n",&syscfg_base_page_size);
    pclose(fp);
	if(syscfg_base_page_size == 4096){
		syscfg_base_pg_idx = PAGE_INDEX_4K;
		t[syscfg_base_pg_idx].page_size = 4096;
	}
	else if(syscfg_base_page_size == 65536) {
		syscfg_base_pg_idx = PAGE_INDEX_64K;
		t[syscfg_base_pg_idx].page_size = 65536;
	}
	else {
		printf("Does not have support of base page size\n");
		return -3;
	}
	t[syscfg_base_pg_idx].supported = TRUE;

	fp=popen("cat /proc/meminfo | grep MemTotal | awk '{print $2}' ","r");
	if (fp == NULL || fp ==  (void *)-1 ) {
		pclose(fp);
		return -3;
	}
	rc = fscanf(fp,"%lu\n",&t[syscfg_base_pg_idx].total_pages);
	if(rc == EOF || rc == 0)
	{
		pclose(fp);
		printf("Could not read MemTotal,rc=%d amd errno=%d\n",rc,errno);
		return -3;
	}
	t[syscfg_base_pg_idx].total_pages = ((t[syscfg_base_pg_idx].total_pages * 1024) / t[syscfg_base_pg_idx].page_size);
	pclose(fp);

	fp=popen("cat /proc/meminfo | grep MemAvailable | awk '{print $2}' ","r");
	if (fp == NULL || fp ==  (void *)-1 ) {
		pclose(fp);
		return -3;
	}
	rc = fscanf(fp,"%lu\n",&t[syscfg_base_pg_idx].free_pages);
	if(rc == EOF || rc == 0)
	{
		pclose(fp);
		fp=popen("cat /proc/meminfo | grep MemFree  | awk '{print $2}' ","r");
		    if (fp == NULL || fp == (void *)-1 ) {
		        pclose(fp);
		        return -3;
		    }
    	rc = fscanf(fp,"%lu\n",&t[syscfg_base_pg_idx].free_pages);
		if(rc == -1) {
			pclose(fp);
			printf("Could not find MemAvailable or MemFree in /proc/meminfo\n");
			return -3;
		}
	}
	t[syscfg_base_pg_idx].free_pages = ((t[syscfg_base_pg_idx].free_pages * 1024) / t[syscfg_base_pg_idx].page_size);
	pclose(fp);
	fp=popen("cat /proc/meminfo | grep Hugepagesize | awk '{print $2}' ","r");
	if (fp == NULL || fp == (void *)-1 ) {
		pclose(fp);
		return -3;
	}

	rc = fscanf(fp,"%d\n",&huge_page_size);
	huge_page_size = (huge_page_size * 1024);

	for(i=2; i<=3;i++){
		t[i].free_pages= 0;
		t[i].supported=FALSE;
	}

	if (huge_page_size == 2097152) {
		t[PAGE_INDEX_2M].page_size = 2097152;
		t[PAGE_INDEX_2M].supported = TRUE;
		fp=popen("cat /proc/meminfo | grep HugePages_Total | awk '{print $2}' ","r");
		if (fp == NULL || fp == (void *)-1 ) {
			pclose(fp);
			return -3;
		}
		rc = fscanf(fp,"%lu\n",&t[PAGE_INDEX_2M].total_pages);
		if(rc == EOF || rc == 0) {
			t[PAGE_INDEX_2M].total_pages= 0;
		}
		pclose(fp);
		fp=popen("cat /proc/meminfo | grep HugePages_Free | awk '{print $2}' ","r");
		if (fp == NULL || fp == (void *)-1 ) {
			pclose(fp);
			return -3;
		}
		rc = fscanf(fp,"%lu\n",&t[PAGE_INDEX_2M].free_pages);
		if(rc == EOF || rc == 0) {
			t[PAGE_INDEX_2M].free_pages= 0;
		}

		pclose(fp);
	}
	else if (huge_page_size == 16777216) {
		t[PAGE_INDEX_16M].page_size = 16777216;
		t[PAGE_INDEX_16M].supported = TRUE;
		fp=popen("cat /proc/meminfo | grep HugePages_Total | awk '{print $2}' ","r");
		if (fp == NULL || fp == (void *)-1 ) {
			pclose(fp);
			return -3;
		}
		rc = fscanf(fp,"%lu\n",&t[PAGE_INDEX_16M].total_pages);
		if(rc == EOF || rc == 0) {
			t[PAGE_INDEX_16M].total_pages= 0;
		}
		pclose(fp);
		fp=popen("cat /proc/meminfo | grep HugePages_Free | awk '{print $2}' ","r");
		if (fp == NULL || fp == (void *)-1 ) {
			pclose(fp);
			return -3;
		}
		rc = fscanf(fp,"%lu\n",&t[PAGE_INDEX_16M].free_pages);
		if(rc == EOF || rc == 0) {
			t[PAGE_INDEX_16M].free_pages= 0;
		}

		pclose(fp);
	}
	return 0;
}

/*Retrieves page size details*/
int get_page_size_update(void)
{
    FILE *fp;
    int i,j,k,num;
    char ch,dest[100];
    unsigned int Pvr;
	htxsyscfg_pages_t *t;
	i = j = k = num = 0;
    t=&(global_ptr->global_memory.page_details[0]);

    Pvr = get_true_cpu_version();
       Pvr = Pvr>>16;

    fp=popen("cut -f 2 /proc/swaps","r");
    if ( fp != NULL ) {
        while(1) {
            ch=fgetc(fp);

            if (feof(fp) ) {
                break;
            }
            if (ferror(fp) ) {
                pclose(fp);
                return(-1);
            }
            if(isdigit(ch)) {
                dest[i++]=ch;
            }
        }
        dest[i] = '\0';
    }
    else {
        pclose(fp);
        return (-1);
    }
    pclose(fp);
    t->swap_space = atoi(dest);  /*Size of swap space in bytes*/

#ifdef __AWAN__
	t->num_sizes=1;
	t->page_sizes[0]=4096;
#else
    if(Pvr == 0x3e)
        fp=popen("od -i /proc/device-tree/cpus/PowerPC,POWER6@0/ibm,processor-page-sizes 2> /dev/null | head -1","r");
    else if (Pvr == 0x3f)
        fp=popen("od -i /proc/device-tree/cpus/PowerPC,POWER7@0/ibm,processor-page-sizes 2> /dev/null | head -1","r");
    else if (Pvr == 0x4a)
        fp=popen("od -i /proc/device-tree/cpus/PowerPC,POWER7@0/ibm,processor-page-sizes 2> /dev/null | head -1","r");
	else if (Pvr == PV_POWER8_MURANO || Pvr == PV_POWER8_VENICE || Pvr == PV_POWER8_PRIME)
        fp=popen("od -i /proc/device-tree/cpus/PowerPC,POWER8@0/ibm,processor-page-sizes 2> /dev/null | head -1","r");
	else if (Pvr == PV_POWER9_NIMBUS || Pvr == PV_POWER9_CUMULUS)
        fp=popen("od -i /proc/device-tree/cpus/PowerPC,POWER9@0/ibm,processor-page-sizes 2> /dev/null | head -1","r");
	else
		return (-1);

    if ( fp != NULL ) {
        while(1) {
            ch=fgetc(fp);

            if (feof(fp) ) {
                break;
            }
            if (ferror(fp) ) {
                pclose(fp);
                return(-1);
            }
            if(isdigit(ch)) {
                i=0;
                while(ch!=' ' && !feof(fp)) {
                    dest[i++]=ch;
                    ch=fgetc(fp);
                }
                dest[i]='\0';
                num=atoi(dest);
                if(num!=0) {
                    t->page_sizes[j]=num;
                    j++;
                }
            }
        }
    }
    else {
        pclose(fp);
        return(-1);
    }
    pclose(fp);
    t->num_sizes=j;    /*Number of page sizes supported*/

    for(i=0; i<t->num_sizes; i++) {
        num=t->page_sizes[i];
        t->page_sizes[i]=1;
        #if 0
        for(k=1; k <= num; k++) {   /*Page sizes supported*/
            t->page_sizes[i]=t->page_sizes[i]*2;
        }
        #endif
}
#endif		/* End of __AWAN__ */
    return(0);
}

int get_page_size(htxsyscfg_pages_t *t)
{
    int i,rc1,rc2,rc1_1;
    rc1=rc2=rc1_1=0;
	rc1 = pthread_rwlock_rdlock(&(global_ptr->global_memory.rw));
    if (rc1!=0  ) {
        if ( rc1 == EDEADLK ) {
                sprintf(msg,"lock inside get_page_size failed with errno=%d\n",rc1);
                hxfmsg(misc_htx_data, -1, HTX_HE_HARD_ERROR, msg);
				return(rc1);
        }
        else if ( rc1 == EAGAIN ) {
			usleep(10);
			rc1_1 = pthread_rwlock_tryrdlock(&(global_ptr->global_memory.rw));
			if(rc1_1!=0){
		            sprintf(msg,"lock inside get_page_size failed in retry with errno=%d\n",rc1_1);
		            hxfmsg(misc_htx_data, -1, HTX_HE_HARD_ERROR, msg);
					return(rc1_1);
			}
        }
		else{
                sprintf(msg,"lock inside get_page_size failed with errno=%d\n",rc1);
				hxfmsg(misc_htx_data, -1, HTX_HE_HARD_ERROR, msg);
				return(rc1);
		}
    }
    for ( i=0; i<MAX_PAGE_SIZES;i++){
	        *t = (global_ptr->global_memory.page_details)[i];
    }


	rc2 = pthread_rwlock_unlock(&(global_ptr->global_memory.rw));
	if (rc2 !=0  ) {
			sprintf(msg,"unlock inside get_page_size failed with errno=%d\n",rc2);
			hxfmsg(misc_htx_data, -1, HTX_HE_HARD_ERROR, msg);
			return(rc2);
	}
	return 0;
}

/*Retrieves memory details*/
#if 0
int get_memory_details_update(void)
{
    int r1,r2,r3;
    r1 = r2 = r3 = 0;
	htxsyscfg_memory_t *t;
    t = &(global_ptr->global_memory);
    //r2 = get_page_size(&(t->page_details)); /*Page sizes supported and swap space*/
/*	r2 = get_page_details(&(t->page_details[0]));
    r3 = get_memory_pools(&(t->mem_pools[0]));
	if(r3 >= 0) {
		t->num_numa_nodes = r3;
	}*/

    return(r3|r1|r2);
}
#endif

int get_memory_details(htxsyscfg_memory_t *t)
{
    int rc1,rc2,rc1_1;
    rc1=rc2=rc1_1=0;
	rc1 = pthread_rwlock_rdlock(&(global_ptr->global_memory.rw));
    if (rc1!=0  ) {
        if ( rc1 == EDEADLK ) {
                sprintf(msg,"lock inside get_memory_details failed with errno=%d\n",rc1);
                hxfmsg(misc_htx_data, -1, HTX_HE_HARD_ERROR, msg);
				return(rc1);
        }
        else if ( rc1 == EAGAIN ) {
			usleep(10);
			rc1_1 = pthread_rwlock_tryrdlock(&(global_ptr->global_memory.rw));
			if(rc1_1!=0){
		            sprintf(msg,"lock inside get_memory_details failed in retry with errno=%d\n",rc1);
		            hxfmsg(misc_htx_data, -1, HTX_HE_HARD_ERROR, msg);
					return(rc1_1);
			}
        }
		else{
                sprintf(msg,"lock inside get_memory_details failed with errno=%d\n",rc1);
				hxfmsg(misc_htx_data, -1, HTX_HE_HARD_ERROR, msg);
				return(rc1);
		}
    }
	*t = (global_ptr->global_memory);

	rc2 = pthread_rwlock_unlock(&(global_ptr->global_memory.rw));
	if (rc2 !=0  ) {
			sprintf(msg,"unlock inside get_memory_details failed with errno=%d\n",rc2);
			hxfmsg(misc_htx_data, -1, HTX_HE_HARD_ERROR, msg);
			return(rc2);
	}
    return 0;
}

/*Reads the pvr register to obtain its value*/
void read_pvr(int *addr)
{
    #ifdef __HTX_LINUX__
     __asm __volatile ("mfpvr 4      \n\t"
                      "stw   4,0(3) \n\t");
    #else
    *addr=get_pvr();
    #endif
}


/*Retrieves L1 details*/
int L1cache_update(void)
{
    int r1,r2,r3,r4;
	r1 = r2 = r3 = r4 = 0;
    char dest[100];
    unsigned int Pvr;
	htxsyscfg_cache_t *t;
    t=&(global_ptr->global_cache);

    Pvr = get_true_cpu_version();
    Pvr = Pvr>>16;

#ifdef __HTX_LE__
    /*L1 data cache size*/
    if(Pvr == 0x3e)
        r1 = get_cmd_op(dest, "od -i /proc/device-tree/cpus/POWER6@0/d-cache-size |  cut -c 15-19 ");
    else if (Pvr == 0x3f )
        r1 = get_cmd_op(dest, "od -i /proc/device-tree/cpus/POWER7@0/d-cache-size |  cut -c 15-19 ");
    else if (Pvr == 0x4a || Pvr == 0x4b )
        r1 = get_cmd_op(dest, "od -i /proc/device-tree/cpus/*POWER*0/d-cache-size |  cut -c 15-19 ");

    t->L1_dsize = atoi(dest);

    /*L1 instruction cache size*/
    if(Pvr == 0x3e )
        r2 = get_cmd_op(dest, "od -i /proc/device-tree/cpus/POWER6@0/i-cache-size |  cut -c 15-19");
    else if (Pvr == 0x3f )
        r2 = get_cmd_op(dest, "od -i /proc/device-tree/cpus/POWER7@0/i-cache-size |  cut -c 15-19");
    else if (Pvr == 0x4a || Pvr == 0x4b )
        r2 = get_cmd_op(dest, "od -i /proc/device-tree/cpus/*POWER*0/i-cache-size |  cut -c 15-19");

    t->L1_isize = atoi(dest);

    /*L1 data cache line size*/
    if(Pvr == 0x3e )
        r3 = get_cmd_op(dest, "od -i /proc/device-tree/cpus/POWER6@0/d-cache-line-size |  cut -c 17-19 ");
    else if (Pvr == 0x3f )
        r3 = get_cmd_op(dest, "od -i /proc/device-tree/cpus/POWER7@0/d-cache-line-size |  cut -c 17-19 ");
    else if (Pvr == 0x4a || Pvr == 0x4b )
        r3 = get_cmd_op(dest, "od -i /proc/device-tree/cpus/*POWER*0/d-cache-line-size |  cut -c 17-19 ");

    t->L1_dline = atoi(dest);

    /*L1 instruction cache line size*/
    if(Pvr == 0x3e )
        r4 = get_cmd_op(dest, "od -i /proc/device-tree/cpus/POWER6@0/i-cache-line-size |  cut -c 17-19");
    else if(Pvr == 0x3f )
        r4 = get_cmd_op(dest, "od -i /proc/device-tree/cpus/POWER7@0/i-cache-line-size |  cut -c 17-19");
    else if (Pvr == 0x4a || Pvr == 0x4b )
        r4 = get_cmd_op(dest, "od -i /proc/device-tree/cpus/*POWER*0/i-cache-line-size |  cut -c 17-19");

    t->L1_iline = atoi(dest);
#else
    /*L1 data cache size*/
    if(Pvr == 0x3e)
        r1 = get_cmd_op(dest, "od -i /proc/device-tree/cpus/PowerPC,POWER6@0/d-cache-size |  cut -c 15-19 ");
    else if (Pvr == 0x3f )
        r1 = get_cmd_op(dest, "od -i /proc/device-tree/cpus/PowerPC,POWER7@0/d-cache-size |  cut -c 15-19 ");
    else if (Pvr == 0x4a || Pvr == 0x4b )
        r1 = get_cmd_op(dest, "od -i /proc/device-tree/cpus/*POWER*0/d-cache-size |  cut -c 15-19 ");

    t->L1_dsize = atoi(dest);

    /*L1 instruction cache size*/
    if(Pvr == 0x3e )
        r2 = get_cmd_op(dest, "od -i /proc/device-tree/cpus/PowerPC,POWER6@0/i-cache-size |  cut -c 15-19");
    else if (Pvr == 0x3f )
        r2 = get_cmd_op(dest, "od -i /proc/device-tree/cpus/PowerPC,POWER7@0/i-cache-size |  cut -c 15-19");
    else if (Pvr == 0x4a || Pvr == 0x4b )
        r2 = get_cmd_op(dest, "od -i /proc/device-tree/cpus/*POWER*0/i-cache-size |  cut -c 15-19");

    t->L1_isize = atoi(dest);

    /*L1 data cache line size*/
    if(Pvr == 0x3e )
        r3 = get_cmd_op(dest, "od -i /proc/device-tree/cpus/PowerPC,POWER6@0/d-cache-line-size |  cut -c 17-19 ");
    else if (Pvr == 0x3f )
        r3 = get_cmd_op(dest, "od -i /proc/device-tree/cpus/PowerPC,POWER7@0/d-cache-line-size |  cut -c 17-19 ");
    else if (Pvr == 0x4a || Pvr == 0x4b )
        r3 = get_cmd_op(dest, "od -i /proc/device-tree/cpus/*POWER*0/d-cache-line-size |  cut -c 17-19 ");

    t->L1_dline = atoi(dest);

    /*L1 instruction cache line size*/
    if(Pvr == 0x3e )
        r4 = get_cmd_op(dest, "od -i /proc/device-tree/cpus/PowerPC,POWER6@0/i-cache-line-size |  cut -c 17-19");
    else if(Pvr == 0x3f )
        r4 = get_cmd_op(dest, "od -i /proc/device-tree/cpus/PowerPC,POWER7@0/i-cache-line-size |  cut -c 17-19");
    else if (Pvr == 0x4a || Pvr == 0x4b )
        r4 = get_cmd_op(dest, "od -i /proc/device-tree/cpus/*POWER*0/i-cache-line-size |  cut -c 17-19");

    t->L1_iline = atoi(dest);
#endif
    if (Pvr == PV_POWER8_MURANO || Pvr == PV_POWER8_VENICE || Pvr == PV_POWER8_PRIME){
        t->L1_dsize = 32*1024;
		t->L1_isize = 64*1024;
		t->L1_dline = 128;
		t->L1_iline = 128;
	}
	else if (Pvr == PV_POWER9_NIMBUS || Pvr == PV_POWER9_CUMULUS){
        t->L1_dsize = 32*1024;
		t->L1_isize = 32*1024;
		t->L1_dline = 128;
		t->L1_iline = 128;
	}
    return ( r1 | r2 | r3 | r4 );
}

int L1cache(htxsyscfg_cache_t *t)
{
    int rc1,rc2,rc1_1;
    rc1=rc2=rc1_1=0;
	rc1 = pthread_rwlock_rdlock(&(global_ptr->global_cache.rw));
    if (rc1!=0  ) {
        if ( rc1 == EDEADLK ) {
                sprintf(msg,"lock inside L1cache failed with errno=%d\n",rc1);
                hxfmsg(misc_htx_data, -1, HTX_HE_HARD_ERROR, msg);
				return(rc1);
        }
        else if ( rc1 == EAGAIN ) {
			usleep(10);
			rc1_1 = pthread_rwlock_tryrdlock(&(global_ptr->global_cache.rw));
			if(rc1_1!=0){
		            sprintf(msg,"lock inside L1cache failed in retry with errno=%d\n",rc1_1);
		            hxfmsg(misc_htx_data, -1, HTX_HE_HARD_ERROR, msg);
					return(rc1_1);
			}
        }
		else{
                sprintf(msg,"lock inside L1cache failed with errno=%d\n",rc1);
				hxfmsg(misc_htx_data, -1, HTX_HE_HARD_ERROR, msg);
				return(rc1);
		}
    }
	*t=(global_ptr->global_cache);

	rc2 = pthread_rwlock_unlock(&(global_ptr->global_cache.rw));
	if (rc2 !=0  ) {
			sprintf(msg,"unlock inside L1cache failed with errno=%d\n",rc2);
			hxfmsg(misc_htx_data, 0, HTX_HE_INFO, msg);
			return(rc2);
	}
    return 0;
}

/*Retrieves L2 & L3 details*/
int L2L3cache_update(void)
{
    unsigned int Pvr;
	htxsyscfg_cache_t *t;
    t=&(global_ptr->global_cache);

    Pvr = get_true_cpu_version();
    Pvr = Pvr>>16;

    /*For P4(GP/GQ) processor */
    if( Pvr == PV_POWER4 || Pvr == PV_POWER4_P ||
        Pvr == PV_PPC970 || Pvr == PV_PPC970_P) {
        t->L2_size = 512*1024;
        t->L2_asc = 8 ;
        t->L2_line = 128 ;

        t->L3_size = 32*1024*1024;
        t->L3_asc = 8 ;
        t->L3_line = 128 ;
    }

    /*For P5 processor*/
    else if( Pvr ==  PV_POWER5 || Pvr == PV_POWER5_P ||
            Pvr == PV_POWER5_PP) {

        t->L2_size = 1920*1024 ; /* 1920KB, Approximately stated in specs as 1.88MB */
        t->L2_asc = 10 ;
        t->L2_line = 128 ;

        t->L3_size = 36*1024*1024;
        t->L3_asc = 12 ;
        t->L3_line = 128 ;
    }

    /*For P6 processor*/
    else if(Pvr == PV_POWER6) {
        t->L2_size = 4*1024*1024 ;
        t->L2_asc = 8 ;
        t->L2_line = 128 ;

        t->L3_size = 32*1024*1024;
        t->L3_asc = 16 ;
        t->L3_line = 128 ;
    }

    /*For P7 processor*/
    else if(Pvr == PV_POWER7)
    {
        t->L2_size = 256*1024 ;
        t->L2_asc = 8 ;
        t->L2_line = 128 ;

        t->L3_size = 4*1024*1024;
        t->L3_asc = 8;
        t->L3_line = 128;
    }

    /*For P7+ processor*/
    else if(Pvr == PV_POWER7PLUS)
    {
        t->L2_size = 256*1024 ;
        t->L2_asc  = 8 ;
        t->L2_line = 128 ;

        t->L3_size = 10*1024*1024;
        t->L3_asc  = 8;
        t->L3_line = 128;
    }

	/* For P8 processor */
	else if (Pvr == PV_POWER8_MURANO || Pvr == PV_POWER8_VENICE || Pvr == PV_POWER8_PRIME) {
		t->L2_size = 512*1024;
		t->L2_asc  = 8;
		t->L2_line = 128;

		t->L3_size = 8*1024*1024;
		t->L3_asc  = 8;
		t->L3_line = 128;
	}
   /* For P9 processor */
    else if(Pvr == PV_POWER9_NIMBUS || Pvr == PV_POWER9_CUMULUS)
    {
        t->L2_size = 512*1024 ;
        t->L2_asc  = 8 ;
        t->L2_line = 128 ;

        t->L3_size = 10*1024*1024;
        t->L3_asc  = 20;
        t->L3_line = 128;
    }
	else  {
        return (-1);
    }

    return(0);
}
int L2L3cache(htxsyscfg_cache_t *t)
{
    int rc1,rc2,rc1_1;
    rc1=rc2=rc1_1=0;
	rc1 = pthread_rwlock_rdlock(&(global_ptr->global_cache.rw));
    if (rc1!=0  ) {
        if ( rc1 == EDEADLK ) {
                sprintf(msg,"lock inside L2L3cache failed with errno=%d\n",rc1);
                hxfmsg(misc_htx_data, -1, HTX_HE_HARD_ERROR, msg);
				return(rc1);
        }
        else if ( rc1 == EAGAIN ) {
			usleep(10);
			rc1_1 = pthread_rwlock_tryrdlock(&(global_ptr->global_cache.rw));
			if(rc1_1!=0){
		            sprintf(msg,"lock inside L2L3cache failed with errno=%d\n",rc1_1);
		            hxfmsg(misc_htx_data, -1, HTX_HE_HARD_ERROR, msg);
					return(rc1_1);
			}
        }
		else{
                sprintf(msg,"lock inside L2L3cache failed with errno=%d\n",rc1);
				hxfmsg(misc_htx_data, -1, HTX_HE_HARD_ERROR, msg);
				return(rc1);
		}
    }

	*t=(global_ptr->global_cache);
	rc2 = pthread_rwlock_unlock(&(global_ptr->global_cache.rw));
	if (rc2 !=0  ) {
			sprintf(msg,"lock inside L2L3cache failed with errno=%d\n",rc2);
			hxfmsg(misc_htx_data, -1, HTX_HE_HARD_ERROR, msg);
			return(rc2);
	}
    return 0;
}


int get_node(unsigned int pvr, int pir) {

    int dd1_bit = 0;

	int rc = 0;
	switch(pvr) {
		case PV_POWER6 :
			rc = P6_GET_NODE(pir);
			break;

		case PV_POWER7:
		case PV_POWER7PLUS:
			rc = P7_GET_NODE(pir);
			break;

		case PV_POWER8_MURANO:
		case PV_POWER8_VENICE:
		case PV_POWER8_PRIME:
			rc = P8_GET_NODE(pir);
			break;
		case PV_POWER9_NIMBUS:
		case PV_POWER9_CUMULUS:
			dd1_bit = get_p9_core_type();
	        if (dd1_bit == DD1_NORMAL_CORE)
				rc = P9_NORMAL_CORE_MODE_GET_NODE(pir);
        	else if (dd1_bit == DD1_FUSED_CORE)
				rc = P9_FUSED_CORE_MODE_GET_NODE(pir);
			break;
		default:
			rc = -1;
			break;
	}
	global_ptr->get_node_value = rc;
	return rc;
}

int get_chip(unsigned int pvr, int pir) {

	int rc = 0;
    int dd1_bit = 0;

	switch(pvr) {
		case PV_POWER6 :
			rc = P6_GET_CHIP(pir);
			break;

		case PV_POWER7:
		case PV_POWER7PLUS:
			rc = P7_GET_CHIP(pir);
			break;

		case PV_POWER8_MURANO:
		case PV_POWER8_VENICE:
		case PV_POWER8_PRIME:
			rc = P8_GET_CHIP(pir);
			break;
		case PV_POWER9_NIMBUS:
        case PV_POWER9_CUMULUS:
			dd1_bit = get_p9_core_type();
            if (dd1_bit == DD1_NORMAL_CORE)
                rc = P9_NORMAL_CORE_MODE_GET_CHIP(pir);
            else if (dd1_bit == DD1_FUSED_CORE)
                rc = P9_FUSED_CORE_MODE_GET_CHIP(pir);
            break;
		default:
			rc = -1;
			break;
	}
	global_ptr->get_chip_value = rc;
	return rc;
}

int get_core(unsigned int pvr, int pir) {

	int rc = 0;
    int dd1_bit = 0;

	switch(pvr) {
		case PV_POWER6 :
			rc = P6_GET_CORE(pir);
			break;

		case PV_POWER7:
		case PV_POWER7PLUS:
			rc = P7_GET_CORE(pir);
			break;

		case PV_POWER8_MURANO:
		case PV_POWER8_VENICE:
		case PV_POWER8_PRIME:
			rc = P8_GET_CORE(pir);
			break;

		case PV_POWER9_NIMBUS:
		case PV_POWER9_CUMULUS:
			dd1_bit = get_p9_core_type();
            if (dd1_bit == DD1_NORMAL_CORE)
                rc = P9_NORMAL_CORE_MODE_GET_CORE(pir);
            else if (dd1_bit == DD1_FUSED_CORE)
                rc = P9_FUSED_CORE_MODE_GET_CORE(pir);
            break;

		default:
			rc = -1;
			break;
	}
	global_ptr->get_core_value = rc;
	return rc;
}

int get_p6_compat_mode(unsigned int Pvr)
{
    unsigned int  temp;
    char dest[256] = {0};

    temp = get_cmd_op(dest, "cat /proc/cpuinfo | grep -i POWER6 | wc -l");
    temp = atoi(dest);

#ifdef DEBUG
printf("Pvr=%x,temp=%d\n", Pvr, temp);
#endif

    if ( ( Pvr == PV_POWER7  || Pvr == PV_POWER7PLUS ) && ( temp > 0 ) )
		return 1;
	else
		return 0;
}


int get_p7_compat_mode(unsigned int Pvr)
{
    unsigned int temp;
    char dest[256] = {0};

    temp = get_cmd_op(dest, "cat /proc/cpuinfo | grep -i POWER7 | wc -l");
    temp = atoi(dest);

#ifdef DEBUG
printf("Pvr=%x,temp=%d\n", Pvr, temp);
#endif

    if ( ( Pvr == PV_POWER8_MURANO || Pvr == PV_POWER8_VENICE || Pvr == PV_POWER8_PRIME || Pvr == PV_POWER9_NIMBUS || Pvr == PV_POWER9_CUMULUS ) && ( temp > 0 ) )
		return 1;
	else
		return 0;
}

int get_p8_compat_mode(unsigned int Pvr)
{
    unsigned int temp;
    char dest[256] = {0};

    temp = get_cmd_op(dest, "cat /proc/cpuinfo | grep -i POWER8 | wc -l");
    temp = atoi(dest);

#ifdef DEBUG
printf("Pvr=%x,temp=%d\n", Pvr, temp);
#endif
    if ( ( Pvr == PV_POWER9_NIMBUS || Pvr == PV_POWER9_CUMULUS ) && ( temp > 0 ) )
                return 1;
        else
                return 0;
}


/*provides endiamness and virtualization type of the architecture*/
int get_env_details_update(void)
{
    unsigned int i = 1;
    int temp;
    char dest[100],fname[100];
	FILE* fp1;
	htxsyscfg_env_details_t* e;

	char htx_home_path[64];
	char *tmp_path_temp;

	tmp_path_temp = getenv("HTX_HOME_DIR");
	if (tmp_path_temp == NULL){
		strcpy(htx_home_path, "/usr/lpp/htx/");
    }
	else{
		strcpy(htx_home_path,getenv("HTX_HOME_DIR"));
	}

    e =&(global_ptr->global_lpar.env_details);
	/* initialize as PVM guest*/
	e->virt_flag = PVM_GUEST;
	strcpy(e->virt_typ,"PVM_GUEST");
	strcpy(e->proc_shared_mode,"no");

	get_cmd_op(dest,"cat /proc/ppc64/lparcfg 2> /dev/null | grep shared_processor_mode | awk -F= '{print $2}'");
	temp = atoi(dest);
	if (temp > 0)
    {
		strcpy(e->proc_shared_mode,"yes");
		e->virt_flag = PVM_PROC_SHARED_GUEST;
	}
    get_cmd_op(dest, "cat /proc/cpuinfo | grep -i PowerNV | wc -l"); /* host machine */
    temp = atoi(dest);
    if (temp > 0)
    {
        e->virt_flag = NV;
		strcpy(e->virt_typ,"NV");
		strcpy(e->proc_shared_mode,"no");
    }

    get_cmd_op(dest, "cat /proc/cpuinfo | grep -i qemu | wc -l");  /* guest machine */

    temp = atoi(dest);
    if (temp > 0)
    {
        sprintf(fname, "%saffinity_yes", htx_home_path);
        if((fp1=fopen(fname,"r")) == NULL)
        {
            e->virt_flag =  KVM_GUEST;
            strcpy(e->virt_typ,"KVM_GUEST");
            strcpy(e->proc_shared_mode,"yes");
        }
        else {
            fclose(fp1);
            e->virt_flag = PVM_GUEST;
            strcpy(e->virt_typ,"KVM_GUEST_CPU_PINNED");
            strcpy(e->proc_shared_mode,"no");
        }
    }

    get_cmd_op(dest, "uname -a | grep -i bml | wc -l");  /*  BML  machine */

    temp = atoi(dest);
    if (temp > 0)
    {
		e->virt_flag =  2;
		strcpy(e->virt_typ,"BML");
		strcpy(e->proc_shared_mode,"no");
    }

    if ( (*(char*)&i) == 1 )
    {
		strcpy(e->endianess,"LE");

	}
	else
	{
		strcpy(e->endianess,"BE");
	}

return 0;
}

void get_env_details(htxsyscfg_env_details_t* e)
{
    int rc1,rc2,rc1_1;
    rc1=rc2=rc1_1=0;
    rc1 = pthread_rwlock_rdlock(&(global_ptr->global_lpar.rw));

    if (rc1!=0  ) {
        if ( rc1 == EDEADLK ) {
                sprintf(msg,"lock inside get_env_details failed with errno=%d\n",rc1);
                hxfmsg(misc_htx_data, -1, HTX_HE_HARD_ERROR, msg);
        }
        else if ( rc1 == EAGAIN ) {
			usleep(10);
			rc1_1 = pthread_rwlock_tryrdlock(&(global_ptr->global_lpar.rw));
			if(rc1_1!=0){
		            sprintf(msg,"lock inside get_env_details failed in retry with errno=%d\n",rc1_1);
		            hxfmsg(misc_htx_data, -1, HTX_HE_HARD_ERROR, msg);
			}
        }
		else{
                sprintf(msg,"lock inside get_env_details failed with errno=%d\n",rc1);
				hxfmsg(misc_htx_data, -1, HTX_HE_HARD_ERROR, msg);
		}
    }
	*e =(global_ptr->global_lpar.env_details);
	rc2=pthread_rwlock_unlock(&(global_ptr->global_lpar.rw));
	if (rc2 !=0  ) {
			sprintf(msg,"unlock inside get_env_details failed with errno=%d\n",rc2);
			hxfmsg(misc_htx_data, -1, HTX_HE_HARD_ERROR, msg);
	}
	return;
}

int get_hardware_config_update(void)
{
#ifdef DEBUG
printf("entering function get_hardware_config,tot_cpus=%d, vir_typ(0=KVM_Guest, 1=NV,2=BML)=%d pvr=%d\n", tot_cpus, type, pvr);
#endif
	unsigned int tot_cpus,pvr;
    int        i, j = 0,k,l,n,num_threads=0;
    int        rc, pir;
    int        l_p[MAX_CPUS] = {-1};
	int		   pcpus[MAX_CPUS] = {-1};
    int        node=0,chip=0,core=0,proc=0;
    int        cpus_per_chip;
    int        cpus_per_node;
    int        chipn=0,coren=0;
    int        node_no = 0;
	int		   type = 0;
    int        pir_array[MAX_THREADS];
    char       msg[100];
	SYS_CONF *sys_conf;
    SYS_CONF   physical_scfg;
    int nnode=0, nchip=0,ncore=0,nlproc=0;
    signed int cpu_index_for_chip[MAX_CHIPS];
    signed int cpu_index_for_node[MAX_NODE];
    signed int cpu_index_for_core[MAX_CORES];
	int core_exclude_flag;
	int x=0,y=0;
	sys_conf=&(global_ptr->syscfg);
    tot_cpus = (global_ptr->global_lpar.cpus.l_cpus);
	type = global_ptr->global_lpar.env_details.virt_flag;
    pvr = global_ptr->global_pvr;
	int excluded_cores_array[MAX_CORES_PER_CHIP][3];

    for(x=0; x<MAX_CORES_PER_CHIP; x++){
        for(y=0;y<3;y++){
            excluded_cores_array[x][y] = -2;
        }
    }

    for(i = 0; i < MAX_CPUS ; i++) {
        l_p[i] = -1;
        pcpus[i] = -1;
	}

    for(i = 0 ; i < tot_cpus ; i++ ) {
		rc = htx_bind_thread(i,-1);
        if(rc < 0) {
			if(rc == -2 || rc == -1){
				sprintf(msg,"syscfg:bind failed for cpu:%d due to hotplug,rc=%d\n",i,rc);
				hxfmsg(misc_htx_data, 0, HTX_HE_INFO, msg);
				if((misc_htx_data != NULL)  && ((strcmp (misc_htx_data->run_type, "OTH") == 0))) {
					printf("syscfg:bind failed for cpu:%d due to hotplug,rc=%d\n",i,rc);
				}
				tot_cpus = get_nprocs();
				i = 0;
				continue;
			}
			else {
				sprintf(msg,"syscfg:bind failed for cpu:%d returning with rc = %d\n",i,rc);
				hxfmsg(misc_htx_data, 0, HTX_HE_INFO, msg);
				if((misc_htx_data != NULL)  && ((strcmp (misc_htx_data->run_type, "OTH") == 0))) {
					printf("syscfg:bind failed for cpu:%d with rc = %d\n",i,rc);
				}
				return -1;
			}
        }
		else if (rc >= 0) {
			pcpus[i] = rc;
		}

        pir = get_cpu_id(rc);
#ifdef DEBUG
	printf("pir = %d\n",pir);
#endif
        if(pir == -1)
        {
            printf("get_cpu_id return %d\n",pir);
            return (-1);
        }
        l_p[i] = pir ;

        #ifdef __HTX_LINUX__
            /* Restore original/default CPU affinity so that it binds to ANY available processor */
            rc = htx_unbind_thread();
        #else
        rc = bindprocessor(BINDPROCESS, getpid(), PROCESSOR_CLASS_ANY);
        #endif
        if(rc == -1) {
                    sprintf(msg,"syscfg:unbind failed for cpu:%d ,rc=%d\n",i,rc);
                    hxfmsg(misc_htx_data, 0, HTX_HE_INFO, msg);
            return(-1);
        }
    }

    for (i=0; i<MAX_THREADS; i++) {
        (sys_conf->Logical_cpus[i]) = -1;
		(sys_conf->Physical_cpus[i]) = -1;
        pir_array[i] = -1;         /* Initialise the PIR array with all -1s */
    }

    for(i=0; i<MAX_THREADS; i++) {
        for(j=0; j<11 ; j++) {
            if(j == 0)
                (sys_conf->duplicate[i][j]) = 0;
            else
                (sys_conf->duplicate[i][j]) = -1;
        }
    }

    /* Initialise the CPU index map for node/chip/core */
    for( i=0 ; i<MAX_NODE ; i++ ) {
        (global_ptr->system_cpu_map.cpus_index_in_node[i].start)    = -1;
        (global_ptr->system_cpu_map.cpus_index_in_node[i].stop)     = -1;
        (global_ptr->system_cpu_map.cpus_index_in_node[i].num_cpus) = 0;
    }

    for( i=0 ; i<MAX_CHIPS ; i++ ) {
        (global_ptr->system_cpu_map.cpus_index_in_chip[i].start)    = -1;
        (global_ptr->system_cpu_map.cpus_index_in_chip[i].stop)     = -1;
        (global_ptr->system_cpu_map.cpus_index_in_chip[i].num_cpus) = 0;
    }

    for( i=0 ; i<MAX_CORES ; i++ ) {
        (global_ptr->system_cpu_map.cpus_index_in_core[i].start)    = -1;
        (global_ptr->system_cpu_map.cpus_index_in_core[i].stop)     = -1;
        (global_ptr->system_cpu_map.cpus_index_in_core[i].num_cpus) = 0;
    }

    for(node = 0; node < MAX_NODE; node++) {
        for(chip = 0; chip < MAX_CHIPS_PER_NODE; chip ++) {
            for(core =0; core < MAX_CORES_PER_CHIP; core ++) {
                for(proc = 0; proc < MAX_CPUS_PER_CORE; proc ++) {
                    physical_scfg.node[node].chip[chip].core[core].lprocs[proc] = -1;
					physical_scfg.node[node].chip[chip].core[core].pprocs[proc] = -1;
                }
                physical_scfg.node[node].chip[chip].core[core].num_procs = 0;
            }
            physical_scfg.node[node].chip[chip].num_cores = 0;
        }
        physical_scfg.node[node].num_chips = 0;
    }
    physical_scfg.num_nodes = 0 ;

    for(node = 0; node < MAX_NODE; node++) {
        for(chip = 0; chip < MAX_CHIPS_PER_NODE; chip ++) {
            for(core =0; core < MAX_CORES_PER_CHIP; core ++) {
                for(proc = 0; proc < MAX_CPUS_PER_CORE; proc ++) {
                    sys_conf->node[node].chip[chip].core[core].lprocs[proc] = -1;
                                        sys_conf->node[node].chip[chip].core[core].pprocs[proc] = -1;
                }
                sys_conf->node[node].chip[chip].core[core].num_procs = 0;
            }
            sys_conf->node[node].chip[chip].num_cores = 0;
        }
        sys_conf->node[node].num_chips = 0;
    }
    sys_conf->num_nodes = 0 ;
    if(type != KVM_GUEST && type != PVM_PROC_SHARED_GUEST)/* for both KVM host and PVM*/
    {
        i = 0;
        while(i < tot_cpus) {

            node = get_node(pvr,l_p[i]);
            chip = get_chip(pvr,l_p[i]);
            core = get_core(pvr,l_p[i]);
            if(node == -1 || core == -1 || chip == -1){
                        sprintf(msg,"syscfg:something wrong with pvr ,node=%d,chip=%d,core=%d\n",node,chip,core);
                    hxfmsg(misc_htx_data, 0, HTX_HE_INFO, msg);
                    return -1;
            }
            if ( pir_array[l_p[i]] != -1 ) {          /* If the pir_array entry is already populated then */
                int temp_count = ++(sys_conf->duplicate[pir_array[l_p[i]]][0]);
                (sys_conf->duplicate[pir_array[l_p[i]]][temp_count]) = i;     /* This is a dupliacate entry for the early proc */
            }
            else {
                pir_array[l_p[i]] = i;
                physical_scfg.node[node].chip[chip].core[core].lprocs[physical_scfg.node[node].chip[chip].core[core].num_procs++] = i ;
                physical_scfg.node[node].chip[chip].core[core].pprocs[physical_scfg.node[node].chip[chip].core[core].num_procs - 1] = pcpus[i];
            }
			#ifdef DEBUG
			DEBUGON("[%d] Node- %d, Chip= %d ,Core= %d, P_NO=%d, L_PNO=%d num_procs=%d \n",__LINE__,node,chip,core, l_p[i], i,physical_scfg.node[node].chip[chip].core[core].num_procs);
			printf("[%d] Pir_array[%d] = %d ",__LINE__,l_p[i],pir_array[l_p[i]]);
			for(j=0;j<=(sys_conf->duplicate[pir_array[l_p[i]]][0]);j++)
            printf("duplicate[%d][%d] = %d ",pir_array[l_p[i]],j,(sys_conf->duplicate[pir_array[l_p[i]]][j]));
			printf("\n");
			#endif
            i++;
        }
    }

    for(i = 0; i < MAX_NODE; i ++) {
        for( j = 0; j < MAX_CHIPS_PER_NODE; j++) {
            for( k = 0; k < MAX_CORES_PER_CHIP; k++) {
                if(physical_scfg.node[i].chip[j].core[k].num_procs > 0) {
                    physical_scfg.node[i].chip[j].num_cores++;
                }
            }
            if(physical_scfg.node[i].chip[j].num_cores > 0) {
                physical_scfg.node[i].num_chips ++;
            }
        }
        if(physical_scfg.node[i].num_chips > 0) {
            physical_scfg.num_nodes ++;
        }
    }

  #ifdef DEBUG
  for(node = 0; node < MAX_NODE; node++) {
        for(chip = 0; chip < MAX_CHIPS_PER_NODE; chip ++) {
            for(core =0; core < MAX_CORES_PER_CHIP; core ++) {
                printf("physical_scfg.node[%d].chip[%d].core[%d].num_procs = %d\n",node,chip,core,physical_scfg.node[node].chip[chip].core[core].num_procs);
            }
        }
    }
  #endif

core_exclude_flag = array_index = x  = 0;
/* for lchip lcore array */
    for(i = 0; i < MAX_NODE; i ++) {
        if(physical_scfg.node[i].num_chips > 0) {
            for( j = 0,nchip=0; j < MAX_CHIPS_PER_NODE; j++) {
                if( physical_scfg.node[i].chip[j].num_cores > 0) {
                     for( k = 0,ncore=0; k < MAX_CORES_PER_CHIP; k++) {
                        if(physical_scfg.node[i].chip[j].core[k].num_procs > 0) {
                            for( l = 0,nlproc=0; l < physical_scfg.node[i].chip[j].core[k].num_procs ; l++) {
                                sys_conf->node[nnode].chip[nchip].lcore[ncore].lprocs[nlproc] = physical_scfg.node[i].chip[j].core[k].lprocs[l];
                                sys_conf->node[nnode].chip[nchip].lcore[ncore].pprocs[nlproc] = physical_scfg.node[i].chip[j].core[k].pprocs[l];
							     for(array_index=0;array_index<MAX_CORES_PER_CHIP;array_index++){
				                    if( (nnode == (sys_conf->numberArray[array_index][0])) && (nchip == (sys_conf->numberArray[array_index][1])) && (ncore == (sys_conf->numberArray[array_index][2]))){
										for(y=0; y<3; y++){
											excluded_cores_array[x][y] = sys_conf->numberArray[array_index][y];
										}
										x++;
									   core_exclude_flag = 1;
									}
								 }
				                if(core_exclude_flag == 1){
                                    sys_conf->node[nnode].chip[nchip].lcore[ncore].lprocs[nlproc] = -1;
                                    sys_conf->node[nnode].chip[nchip].lcore[ncore].pprocs[nlproc] = -1;
                                }
                                else{
                                    sys_conf->node[nnode].chip[nchip].lcore[ncore].lprocs[nlproc] = physical_scfg.node[i].chip[j].core[k].lprocs[l];
                                    sys_conf->node[nnode].chip[nchip].lcore[ncore].pprocs[nlproc] = physical_scfg.node[i].chip[j].core[k].pprocs[l];
                                }
                                nlproc++;
                            }
							if(core_exclude_flag == 1){
                                sys_conf->node[nnode].chip[nchip].lcore[ncore].num_procs = 0;
                            }
                            else{
                                sys_conf->node[nnode].chip[nchip].lcore[ncore].num_procs = physical_scfg.node[i].chip[j].core[k].num_procs;
                            }
                            ncore++;
						    }

							if(core_exclude_flag == 1)
							{
								core_exclude_flag = 0;
								 sys_conf->node[nnode].chip[nchip].num_cores_excluded++;
							}
							else{
							}
                    }
					sys_conf->node[nnode].chip[nchip].num_cores = physical_scfg.node[i].chip[j].num_cores - sys_conf->node[nnode].chip[nchip].num_cores_excluded;
                    nchip++;
                }
            }
            sys_conf->node[nnode].num_chips = physical_scfg.node[i].num_chips;
            nnode++;
         }
    }

    sys_conf->num_nodes = physical_scfg.num_nodes;

nnode = 0;

/* for adjusting the number of lchip-lcore array with chip-core array*/
	for(i = 0; i < MAX_NODE; i ++) {
        if(sys_conf->node[i].num_chips > 0) {
            for( j = 0,nchip=0; j < MAX_CHIPS_PER_NODE; j++) {
                if( sys_conf->node[i].chip[j].num_cores > 0) {
                     for( k = 0,ncore=0; k < MAX_CORES_PER_CHIP; k++) {
                        if(sys_conf->node[i].chip[j].lcore[k].num_procs > 0) {
                            for( l = 0,nlproc=0; l < sys_conf->node[i].chip[j].lcore[k].num_procs ; l++) {
                                sys_conf->node[nnode].chip[nchip].core[ncore].lprocs[nlproc] = sys_conf->node[i].chip[j].lcore[k].lprocs[l];
                                sys_conf->node[nnode].chip[nchip].core[ncore].pprocs[nlproc] = sys_conf->node[i].chip[j].lcore[k].pprocs[l];
                                nlproc++;
                            }
                            sys_conf->node[nnode].chip[nchip].core[ncore].num_procs = sys_conf->node[i].chip[j].lcore[k].num_procs;
                            ncore++;
                        }
                    }
                    sys_conf->node[nnode].chip[nchip].num_cores = sys_conf->node[i].chip[j].num_cores;
                    nchip++;
                }
            }
            sys_conf->node[nnode].num_chips = sys_conf->node[i].num_chips;
            nnode++;
         }
    }
    sys_conf->num_nodes = physical_scfg.num_nodes;


    for(i = 0; i < sys_conf->num_nodes; i ++) {
        for( j = 0; j < sys_conf->node[i].num_chips; j++) {
            for( k = 0; k < sys_conf->node[i].chip[j].num_cores; k++) {
                if(sys_conf->node[i].chip[j].core[k].num_procs > 0) {
                    for( l = 0; l <sys_conf->node[i].chip[j].core[k].num_procs ; l++) {
                       sys_conf->Logical_cpus[num_threads++] = sys_conf->node[i].chip[j].core[k].lprocs[l];
                       sys_conf->Physical_cpus[num_threads -1] = sys_conf->node[i].chip[j].core[k].pprocs[l];
                    }
                }
            }
        }
    }

/* eliminating the duplicate entries in excluded_cores_array */
n = MAX_CORES_PER_CHIP - 1;
 for(x=0; x < n; x++)
   {
      for(y=x+1; y < n; )
      {
         if((excluded_cores_array[y][0] == excluded_cores_array[x][0]) && (excluded_cores_array[y][1] == excluded_cores_array[x][1]) && (excluded_cores_array[y][2] == excluded_cores_array[x][2]))
         {
            for(k=y; k < n;k++)
            {
               excluded_cores_array[k][0] = excluded_cores_array[k+1][0];
				excluded_cores_array[k][1] = excluded_cores_array[k+1][1];
				excluded_cores_array[k][2] = excluded_cores_array[k+1][2];

            }
            n--;
         }
         else {
            y++;
         }
      }
   }

/*copying it to the global structure */
    for(x=0; x<MAX_CORES_PER_CHIP; x++){
        for(y=0;y<3;y++){
            global_ptr->syscfg.numberArray[x][y] = excluded_cores_array[x][y] ;
        }
    }



    cpu_index_for_core[0] = 0;
    cpu_index_for_chip[0] = 0;
    cpu_index_for_node[0] = 0;


    for( i=0,node_no=0 ; i<sys_conf->num_nodes ; i++,node_no++ ) {
        cpus_per_node = 0;
        for( j=0 ; j<sys_conf->node[i].num_chips ; j++,chipn++ ) {
            cpus_per_chip = 0;
            for( k=0 ; k<sys_conf->node[i].chip[j].num_cores; k++,coren++) {
                cpu_index_for_core[coren+1] = cpu_index_for_core[coren] + sys_conf->node[i].chip[j].core[k].num_procs;

                global_ptr->system_cpu_map.cpus_index_in_core[coren].start    = cpu_index_for_core[coren];     /* Populate the CPU map for core */
                global_ptr->system_cpu_map.cpus_index_in_core[coren].stop     = cpu_index_for_core[coren+1];
                global_ptr->system_cpu_map.cpus_index_in_core[coren].num_cpus = cpu_index_for_core[coren+1] - cpu_index_for_core[coren];

                cpus_per_node += sys_conf->node[i].chip[j].core[k].num_procs;
                cpus_per_chip += sys_conf->node[i].chip[j].core[k].num_procs;
            }
        cpu_index_for_chip[chipn+1] = cpu_index_for_chip[chipn] + cpus_per_chip;

        global_ptr->system_cpu_map.cpus_index_in_chip[chipn].start    = cpu_index_for_chip[chipn];             /* Populate the CPU map for chip */
        global_ptr->system_cpu_map.cpus_index_in_chip[chipn].stop     = cpu_index_for_chip[chipn+1];
        global_ptr->system_cpu_map.cpus_index_in_chip[chipn].num_cpus = cpu_index_for_chip[chipn+1] - cpu_index_for_chip[chipn];
        }

    cpu_index_for_node[node_no+1] = cpu_index_for_node[node_no] + cpus_per_node;

    global_ptr->system_cpu_map.cpus_index_in_node[node_no].start    = cpu_index_for_node[node_no];                 /* Populate the CPU map for node */
    global_ptr->system_cpu_map.cpus_index_in_node[node_no].stop     = cpu_index_for_node[node_no+1];
    global_ptr->system_cpu_map.cpus_index_in_node[node_no].num_cpus = cpu_index_for_node[node_no+1] - cpu_index_for_node[node_no];
    }

#ifdef DEBUG
printf("Exiting function get_hardware_config\n");
#endif

    return 0 ;
}

int get_hardware_config(SYS_CONF *sys_conf,unsigned int tot_cpus, unsigned int pvr,int type){
    int rc1,rc2,rc1_1;
    rc1=rc2=rc1_1=0;
	rc1 = pthread_rwlock_rdlock(&(global_ptr->syscfg.rw));

    if (rc1!=0  ) {
        if ( rc1 == EDEADLK ) {
                sprintf(msg,"lock inside get_hardware_config failed with errno=%d\n",rc1);
                hxfmsg(misc_htx_data, -1, HTX_HE_HARD_ERROR, msg);
				return(rc1);
        }
        else if ( rc1 == EAGAIN ) {
			usleep(10);
			rc1_1 = pthread_rwlock_tryrdlock(&(global_ptr->syscfg.rw));
			if(rc1_1!=0){
		            sprintf(msg,"lock inside get_hardware_config failed in retry with errno=%d\n",rc1_1);
		            hxfmsg(misc_htx_data, -1, HTX_HE_HARD_ERROR, msg);
					return(rc1_1);
			}
        }
		else{
                sprintf(msg,"lock inside get_hardware_config failed with errno=%d\n",rc1);
				hxfmsg(misc_htx_data, -1, HTX_HE_HARD_ERROR, msg);
				return(rc1);
		}
    }
	*sys_conf = (global_ptr->syscfg);

	rc2 = pthread_rwlock_unlock(&(global_ptr->syscfg.rw));
	if (rc2 !=0  ) {
			sprintf(msg,"unlock inside get_hardware_config failed with errno=%d\n",rc2);
			hxfmsg(misc_htx_data, -1, HTX_HE_HARD_ERROR, msg);
			return(rc2);
	}
    return 0;
}

/* Fetches CPUs in a node                       */
/* Returns number of CPUs in a node on success, */
/* -1 on error                                  */
int get_cpus_in_node(int node_no,signed int *cpus_in_node)
{
    int              i,j;

    for(i=0 ; i<MAX_THREADS_PER_NODE ; i++) {
     cpus_in_node[i] = -1;
    }

    if( node_no > global_ptr->syscfg.num_nodes ) {
			sprintf(msg,"syscfg:node num:%d passed is greater than total nodes:%d\n",node_no,global_ptr->syscfg.num_nodes);
		hxfmsg(misc_htx_data, 0, HTX_HE_INFO, msg);
        return (-1);
    }

    for( i=global_ptr->system_cpu_map.cpus_index_in_node[node_no].start,j=0 ; i<global_ptr->system_cpu_map.cpus_index_in_node[node_no].stop ; i++,j++ ) {
        cpus_in_node[j] = (global_ptr->syscfg.Logical_cpus[i]);
    }

    return (global_ptr->system_cpu_map.cpus_index_in_node[node_no].num_cpus);
}

/* Fetches CPUs in a chip.                        */
/* Returns number of CPUS in the chip on success, */
/* -1 on error                                    */
int get_cpus_in_chip(int chip_no,signed int *cpus_in_chip)
{
    int              i,j;

    for(i=0; i<MAX_THREADS_PER_CHIP; i++) {
        cpus_in_chip[i] = -1;
    }

    if( chip_no>MAX_CHIPS ){
			sprintf(msg,"syscfg:chip num:%d passed is greater than max chips:%d\n",chip_no,MAX_CHIPS);
			hxfmsg(misc_htx_data, 0, HTX_HE_INFO, msg);
        return (-1);
	}

    for( i=global_ptr->system_cpu_map.cpus_index_in_chip[chip_no].start,j=0 ; i<global_ptr->system_cpu_map.cpus_index_in_chip[chip_no].stop ; i++,j++ ) {
        cpus_in_chip[j] = (global_ptr->syscfg.Logical_cpus[i]);
    }

    return (global_ptr->system_cpu_map.cpus_index_in_chip[chip_no].num_cpus);
}


/* Retrieves Number of Cpus, chips, cores and nodes in hardware */
/* Returns 0 on success, -1 on error                            */
int get_hardware_stat_update(void)
{
    int              i,j,k;
    int              total_cpus,core=0,socket=0;
	char dest[100];
    SYS_STAT *sys_stat;
    sys_stat = &(global_ptr->system_cpu_map.stat);
    total_cpus = (global_ptr->global_lpar.cpus.l_cpus);
	sys_stat->nodes = 0;
    sys_stat->chips = 0;
    sys_stat->cores = 0;

    for (i=0; i<global_ptr->syscfg.num_nodes; i++) {
        sys_stat->nodes++;
        for(j=0; j<global_ptr->syscfg.node[i].num_chips; j++) {
            sys_stat->chips++;
            for(k=0; k<global_ptr->syscfg.node[i].chip[j].num_cores; k++) {
                sys_stat->cores++;
            }
        }
    }
    sys_stat->cpus = total_cpus;
	if(sys_stat->cores == 0) {
        if(get_cmd_op(dest,"lscpu | grep Core | cut -f 2 -d :") == -1)
			return -1;
        core = atoi(dest);
        if(get_cmd_op(dest,"lscpu | grep Socket | cut -f 2 -d :") == -1)
			return -1;
        socket = atoi(dest);
        sys_stat->cores = core * socket;
	}

    return 0;
}

int get_hardware_stat(SYS_STAT *sys_stat)
{
    int rc1,rc2,rc1_1;
    rc1=rc2=rc1_1=0;
	rc1 = pthread_rwlock_rdlock(&(global_ptr->system_cpu_map.stat.rw));

    if (rc1!=0  ) {
        if ( rc1 == EDEADLK ) {
                sprintf(msg,"lock inside get_hardware_stat failed with errno=%d\n",rc1);
                hxfmsg(misc_htx_data, -1, HTX_HE_HARD_ERROR, msg);
				return(rc1);
        }
        else if ( rc1 == EAGAIN ) {
			usleep(10);
			rc1_1 = pthread_rwlock_tryrdlock(&(global_ptr->system_cpu_map.stat.rw));
			if(rc1_1!=0){
		            sprintf(msg,"lock inside get_hardware_stat failed with errno=%d\n",rc1_1);
		            hxfmsg(misc_htx_data, -1, HTX_HE_HARD_ERROR, msg);
					return(rc1_1);
			}
        }
		else{
                sprintf(msg,"lock inside get_hardware_stat failed with errno=%d\n",rc1);
				hxfmsg(misc_htx_data, -1, HTX_HE_HARD_ERROR, msg);
				return(rc1);
		}
    }
	*sys_stat = (global_ptr->system_cpu_map.stat);

	rc2 = pthread_rwlock_unlock(&(global_ptr->system_cpu_map.stat.rw));
	if (rc2 !=0  ) {
			sprintf(msg,"unlock inside get_hardware_stat failed with errno=%d\n",rc2);
			hxfmsg(misc_htx_data, -1, HTX_HE_HARD_ERROR, msg);
			return(rc2);
	}
    return 0;
}

/* Fetches CPUS in a core                         */
/* Input is core number                           */
/* and an integer array to return the cpus        */
/* Returns number of CPUs in the core on success, */
/* -1 on error                                    */
int get_cpus_in_core(int core_no,signed int *cpus_in_core)
{
    int              i,j;

    for(i=0; i<MAX_THREADS_PER_CORE; i++) {
        cpus_in_core[i] = -1;
    }

    if( core_no>MAX_CORES ){
			sprintf(msg,"syscfg:core num:%d passed is greater than max cores:%d\n",core_no,MAX_CORES);
		hxfmsg(misc_htx_data, 0, HTX_HE_INFO, msg);
        return (-1);
	}

    for( i=global_ptr->system_cpu_map.cpus_index_in_core[core_no].start,j=0 ; i<global_ptr->system_cpu_map.cpus_index_in_core[core_no].stop ; i++,j++ ) {
        cpus_in_core[j] = (global_ptr->syscfg.Logical_cpus[i]);
    }
    return (global_ptr->system_cpu_map.cpus_index_in_core[core_no].num_cpus);
}

int get_phy_cpus_in_core(int core_no,signed int *cpus_in_core)
{
    int              i,j;

    for(i=0; i<MAX_THREADS_PER_CORE; i++) {
        cpus_in_core[i] = -1;
    }

    if( core_no>MAX_CORES ) {
			sprintf(msg,"syscfg:core num:%d passed is greater than max cores:%d\n",core_no,MAX_CORES);
			hxfmsg(misc_htx_data, 0, HTX_HE_INFO, msg);
        return (-1);
	}
    for( i=global_ptr->system_cpu_map.cpus_index_in_core[core_no].start,j=0 ; i<global_ptr->system_cpu_map.cpus_index_in_core[core_no].stop ; i++,j++ ) {
        cpus_in_core[j] = (global_ptr->syscfg.Physical_cpus[i]);
    }
    return (global_ptr->system_cpu_map.cpus_index_in_core[core_no].num_cpus);
}
/* Fetches CPUs in a chip.                        */
/* Returns number of CPUS in the chip on success, */
/* -1 on error                                    */
int get_phy_cpus_in_chip(int chip_no,signed int *cpus_in_chip)
{
    int              i,j;

    for(i=0; i<MAX_THREADS_PER_CHIP; i++) {
        cpus_in_chip[i] = -1;
    }

    if( chip_no>MAX_CHIPS ){
			sprintf(msg,"syscfg:chip num:%d passed is greater than max chips:%d\n",chip_no,MAX_CHIPS);
			hxfmsg(misc_htx_data, 0, HTX_HE_INFO, msg);
        return (-1);
	}

    for( i=global_ptr->system_cpu_map.cpus_index_in_chip[chip_no].start,j=0 ; i<global_ptr->system_cpu_map.cpus_index_in_chip[chip_no].stop ; i++,j++ ) {
        cpus_in_chip[j] = (global_ptr->syscfg.Physical_cpus[i]);
    }

    return (global_ptr->system_cpu_map.cpus_index_in_chip[chip_no].num_cpus);
}

/* Retrieves PVR value
   In AIX, it is just a wrapper function for the getPvr function.
   In Linux, it uses inline assembly to get the value from SPR 287
*/
/*unsigned int get_pvr(void)
{
    unsigned int pvr = 0;
    #ifdef __HTX_LINUX__
    __asm __volatile ("mfspr %0, 287" : "=r" (pvr));
    #else
    pvr = (unsigned int) getPvr();
    #endif

    return pvr;
}*/
int get_cpu_id(int cpu_number)
{
    FILE * FP = fopen("/dev/miscchar","r");
    unsigned int pir;
    FILE *fp;
    char file_name[50];
    int rc = 0,fd,rc1;

    if (FP == NULL)
    {
        #ifdef NO_PIR_SYSFS_SUPPORT
            sprintf(msg,"syscfg:Miscex is not loaded and Sysfs doesnot have pir file,exiting\n");
            hxfmsg(misc_htx_data, 0, HTX_HE_INFO, msg);
        return(-1);
        #endif

    sprintf(file_name, "/sys/devices/system/cpu/cpu%d/pir", cpu_number);

    fp = fopen(file_name, "r");
    if ( fp == NULL)
    {
        rc = retry_open_calls(&fp,file_name,"r");
        if(rc < 0){
            return -1;
        }
    }
    rc1 = fscanf(fp, "%x", &pir);
	if(rc1==0){
		sprintf(msg,"[%d][%s]Failed to open with errno=%d\n",__LINE__,__FUNCTION__,errno);
		hxfmsg(misc_htx_data, 0, HTX_HE_INFO, msg);
	}
    fclose(fp);

    }
    else
    {
        fclose(FP);
        fd = open("/dev/miscchar", O_RDWR);
        if (fd < 0)
        {
                sprintf(msg,"[%d][%s]misc:Error opening file /dev/miscchar\n",__LINE__,__FUNCTION__);
                hxfmsg(misc_htx_data, 0, HTX_HE_INFO, msg);
                return -1;
        }

        pir = ioctl(fd, PROCESSOR_ID, NULL);
 close(fd);

    }
    return pir;
}


int get_cpu_revision(void)
{
    int pvr_last_two_bytes = 0;
    pvr_last_two_bytes = (int) get_cpu_version();
    pvr_last_two_bytes = pvr_last_two_bytes & 0x0000FFFF;
    return pvr_last_two_bytes;
}

int get_true_cpu_revision(void)
{
    int pvr_last_two_bytes = 0;
    pvr_last_two_bytes = (int) get_true_cpu_version();
    pvr_last_two_bytes = pvr_last_two_bytes & 0x0000FFFF;
    return pvr_last_two_bytes;
}

/* API to see if the chip is in fused or normal core mode   */
/* DD1_NORMAL_CORE = 1 and DD1_FUSED_CORE = 0         */
/* check for DD1_NORMAL_CORE or DD1_FUSED_CORE in exer*/
int get_p9_core_type(void)
{
    int pvr = 0;
    int dd1_bit = 0;
    pvr = global_ptr->global_pvr;
    int pvr_full = (int)get_true_cpu_version();
	if (pvr == PV_POWER9_NIMBUS || pvr == PV_POWER9_CUMULUS){
		int pvr_revision = (pvr_full >> 8 & 0x0F) ; /* get the version value DD1 or DD2 */
		dd1_bit = ((pvr_full >> 12)  & 0x01); /* 19th bit in pvr signifies the fused/normal core mode*/
		if(pvr_revision  == 1 ){
			if ( strcasecmp(global_ptr->global_lpar.env_details.virt_typ,"NV") == 0)
				return(DD1_NORMAL_CORE);
			else if ( strcasecmp(global_ptr->global_lpar.env_details.virt_typ,"KVM_GUEST") == 0)
				return(DD1_NORMAL_CORE);
			else
				return(dd1_bit);
		}
		else if (pvr_revision  ==  2 ){
			return(dd1_bit);
       	}
	}
	else
		return(DD1_FUSED_CORE);
}

int get_num_of_nodes_in_sys(void)
{
	return(global_ptr->syscfg.num_nodes);
}

int get_num_of_chips_in_node(int node_num)
{
    if ( node_num > global_ptr->syscfg.num_nodes) {
        return -1;
    }
	return(global_ptr->syscfg.node[node_num].num_chips);
}

int get_num_of_cores_in_chip(int node_num, int chip_num)
{
    if ((node_num > global_ptr->syscfg.num_nodes) || (chip_num > global_ptr->syscfg.node[node_num].num_chips)) {
        return -1;
    }
	return(global_ptr->syscfg.node[node_num].chip[chip_num].num_cores);
}

int get_num_of_cpus_in_core(int node_num, int chip_num, int core_num)
{
    if ((node_num > global_ptr->syscfg.num_nodes) ||
        (chip_num > global_ptr->syscfg.node[node_num].num_chips) ||
        (core_num > global_ptr->syscfg.node[node_num].chip[chip_num].num_cores)) {
        return -1;
    }
    return(global_ptr->syscfg.node[node_num].chip[chip_num].core[core_num].num_procs);
}

int get_logical_cpu_num(int node_num, int chip_num, int core_num, int thread_num)
{
	int logical_cpu_num = 0;

    if ((node_num > global_ptr->syscfg.num_nodes) ||
        (chip_num > global_ptr->syscfg.node[node_num].num_chips) ||
        (core_num > global_ptr->syscfg.node[node_num].chip[chip_num].num_cores)) {
        return -1;
    }

    logical_cpu_num = global_ptr->syscfg.node[node_num].chip[chip_num].core[core_num].lprocs[thread_num];
    return logical_cpu_num;
}

/* This function is used to repopulate the system configuration structure.      */
/* Returns 0 on success.                                                        */

int repopulate_syscfg(struct htx_data *p_htx_data)
{
    int rc = 0;
    int temp_shm_key=0;

    if ((misc_htx_data != NULL)  && ((strcmp (misc_htx_data->run_type, "OTH") == 0))) {
		rc = init_syscfg_with_malloc();
        if ( rc ){
				sprintf(msg,"Error in repopulate_syscfg while run from cmd line. errno=%d\n", rc);
                hxfmsg(misc_htx_data, 0, HTX_HE_INFO, msg);
		}
		return(rc);
    }

	else{

	    static  struct  sembuf  syscfg_repopulate_halt_sops[1] =
	    {
	        {
	            /* HE wait for syscfg update to complete */
		        (unsigned short) SEM_POSITION_SYSCFG,
			    (short) 0,
				SEM_UNDO
	        }
	    };

	    while( ((p_shm_hdr)->shutdown == 0) && (semop(sem_id, syscfg_repopulate_halt_sops, 1) == -1) && (errno == EINTR) );

		if( global_ptr == NULL ){

			/* Check if shm is already created or not. It will not be present when running from command line */
			temp_shm_key = shmget(SYSCFG_SHM_KEY,(sizeof(GLOBAL_SYSCFG)),(IPC_CREAT | IPC_EXCL | S_IRWXU| S_IRWXG | S_IRWXO));
			if ( temp_shm_key == -1 && errno == EEXIST ) {
				/*shm already existing*/
				rc = attach_to_syscfg();
			if ( rc ){
	                sprintf(msg,"Error in repopulate_syscfg. errno=%d\n", rc);
		            hxfmsg(misc_htx_data, 0, HTX_HE_INFO, msg);
				return(rc);
			}
		}
		else if ((temp_shm_key != -1)) {
                sprintf(msg,"Error in shmget:shm not existing with errno=%d\n", errno);
                hxfmsg(misc_htx_data, 0, HTX_HE_INFO, msg);
			return(109);
		}

			else {
					sprintf(msg,"Error in shmget when global_ptr is NULL. errno=%d\n", errno);
					hxfmsg(misc_htx_data, 0, HTX_HE_INFO, msg);
				return(110);
			}
		}
	}
	return(rc);
}


/* This function returns the SMT status of a particular core.        */
/* Input is the core number.                                         */
/* Returns the smt status of core (1,2,4) on success or -1 on error. */

int get_smt_status(int core_no)
{
    int              smt = -1;
    smt = global_ptr->system_cpu_map.cpus_index_in_core[core_no].num_cpus;

    return (smt);
}

int get_cores_in_chip(int node_num, int chip_num, int *core_number_list )
{
    int i, j, k, index = 0;
    int got_details = 0, core = 0;

    if ((node_num > global_ptr->syscfg.num_nodes) || (chip_num > global_ptr->syscfg.node[node_num].num_chips)) {
        return -1;
    }
    /* Loop through the syscfg structure  */
    /* Loop through nodes until the node*/
    /* number equals the node that*/
    /* we are interested. While the core  */
    /* number is always incremented. In   */
    /* this way, when we reach our node,  */
    /* the core numbers are actually the  */
    /* cores in the node*/

    for( i = 0 ; i < global_ptr->syscfg.num_nodes && got_details == 0 ; i++) {
        for( j=0 ; j<global_ptr->syscfg.node[i].num_chips ; j++ ) {
            if((i == node_num) && (j == chip_num)) {
                for( k=0 ; k<global_ptr->syscfg.node[i].chip[j].num_cores ; k++, core++) {
                    core_number_list[index++] = core;
                }
                got_details = 1;
                break;
            } else {
                core += global_ptr->syscfg.node[i].chip[j].num_cores;
            }
        }
    }
    return index;
}

int get_chips_in_node(int node_num, int *chip_number_list )
{
    int i, j, chip = 0, index = 0;

    if ( node_num > global_ptr->syscfg.num_nodes) {
        return -1;
    }
    /* Loop through the syscfg structure  */
    /* Loop through nodes until the node*/
    /* number equals the node that*/
    /* we are interested. While the chip  */
    /* number is always incremented. In   */
    /* this way, when we reach our node,  */
    /* the chip numbers are actually the  */
    /* chips in the node*/

    for( i = 0 ; i < global_ptr->syscfg.num_nodes ; i++) {
        if (i == node_num) {
             for( j=0 ; j<global_ptr->syscfg.node[i].num_chips ; j++, chip++ ) {
                chip_number_list[index++] = chip;
            }
            break;
        } else {
            chip += global_ptr->syscfg.node[i].num_chips;
        }
    }
    return index;
}

int get_cores_in_node(int node_num, int *core_number_list )
{
    int              node=0,chip=0,core=0,index=0;
    int              i,j,k;


    /* Loop through the syscfg structure  */
    /* Loop through nodes until the node*/
    /* number equals the node that*/
    /* we are interested. While the core  */
    /* number is always incremented. In   */
    /* this way, when we reach our node,  */
    /* the core numbers are actually the  */
    /* cores in the node*/

    for( i=0 ; i<global_ptr->syscfg.num_nodes ; i++,node++) {
        for( j=0 ; j<global_ptr->syscfg.node[i].num_chips ; j++,chip++ ) {
                for( k=0 ; k<global_ptr->syscfg.node[i].chip[j].num_cores ; k++) {
					if(i == node_num)
                        core_number_list[index++] = core;
					core++;
				}
		}
		if(i > node_num)
			break;
    }

    return index;
}


int get_cpu_map(int type, CPU_MAP *cpu_map, int instance_number) {

    if( type == M_NODE ) {
        cpu_map->start    = global_ptr->system_cpu_map.cpus_index_in_node[instance_number].start;
        cpu_map->stop     = global_ptr->system_cpu_map.cpus_index_in_node[instance_number].stop;
        cpu_map->num_cpus = global_ptr->system_cpu_map.cpus_index_in_node[instance_number].num_cpus;
    }
    else if( type == M_CHIP ) {
        cpu_map->start    = global_ptr->system_cpu_map.cpus_index_in_chip[instance_number].start;
        cpu_map->stop     = global_ptr->system_cpu_map.cpus_index_in_chip[instance_number].stop;
        cpu_map->num_cpus = global_ptr->system_cpu_map.cpus_index_in_chip[instance_number].num_cpus;

    }
    else if( type == M_CORE ) {
        cpu_map->start    = global_ptr->system_cpu_map.cpus_index_in_core[instance_number].start;
        cpu_map->stop     = global_ptr->system_cpu_map.cpus_index_in_core[instance_number].stop;
        cpu_map->num_cpus = global_ptr->system_cpu_map.cpus_index_in_core[instance_number].num_cpus;
    }

    return 0;
}

int get_core_info(int *core_number_list, int instance)
{
    int              chip=0,core=0,index=0,ret=-1;
    int              i,j,k;

    /* Loop through the syscfg structure  */
    /* Loop through chips until the chip  */
    /* number equals the instance of chip */
    /* we are interested. While the core  */
    /* number is always incremented. In   */
    /* this way, when we reach our chip,  */
    /* the core numbers are actually the  */
    /* cores in the chip                  */

    for( i=0 ; i<global_ptr->syscfg.num_nodes ; i++) {
        for( j=0 ; j<global_ptr->syscfg.node[i].num_chips ; j++,chip++ ) {
            for( k=0 ; k<global_ptr->syscfg.node[i].chip[j].num_cores ; k++,core++) {
                if ( chip==instance ) {
                    core_number_list[index++] = core;
                    ret                       =global_ptr->syscfg.node[i].chip[j].num_cores;
                }
            }
        }
    }
    return (ret);
}

/* Returns number of CPUs using sysfs on BML-AWAN */
/* procfs is not available on BML-AWAN runs */

int get_nprocs_awan (void)
{
        FILE *fp;
        int n_procs=1,rc;

        printf("Detecting number of cpus on AWAN\n");

        fp=popen("ls -ld /sys/devices/system/cpu/cpu[0-9]* | wc -l", "r");

        if(fp == NULL) {
                printf("popen failed in get_nprocs_awan function\n");
                printf("Assuming CPUs = 1\n");
                fflush(stdout);
                return n_procs;
        }

        rc = fscanf(fp, "%d", &n_procs);
        if(rc==0){
                sprintf(msg,"[%d][%s]Failed to open file with errno=%d\n",__LINE__,__FUNCTION__,errno);
                hxfmsg(misc_htx_data, 0, HTX_HE_INFO, msg);
        }

        if(n_procs == 0) {
                printf("Number of cpus couldn't be determined.\n");
                printf("Assuming CPUs = 1\n");
                fflush(stdout);
                return n_procs;
        }

        pclose(fp);

        printf("n_procs = %d\n",n_procs);
        fflush(stdout);
		return n_procs;
}


/* Returns number of SMT threads using sysfs */

int get_smt_awan (void)
{

		FILE *fp;
        int i, smt=1, start_cpu=0, end_cpu=0;
        char c, on_cpu[10], smt_str[10], fname[100], on_cpus_list[1024];

        printf("Detecting SMT on AWAN.\n");

        /* Find first online cpu */
        fp = fopen("/sys/devices/system/cpu/online", "r");
        if(fp == NULL) {
                printf("Could not open file /sys/devices/system/cpu/online\n");
                printf("errno = %d, %s\n",errno, strerror(errno));
                printf("Assuming SMT=1\n");
                fflush(stdout);
                return smt;
        }

        if(fgets(on_cpus_list, 1024, fp) != NULL) {
                printf("Fron file -- /sys/devices/system/cpu/online\n");
                printf("Online Cpus = %s",on_cpus_list);
                fclose(fp);
        } else {
                printf("Could not get list of online cpus.\n");
                printf("Assuming SMT=1\n");
                fflush(stdout);
                return smt;
        }

        i=0;

        while (sscanf((char *) &on_cpus_list[i], "%c", &c)!= EOF)  {
                if (!isdigit(c)) { break ;}
                on_cpu[i]=c;
                i++;
        }
        on_cpu[i]='\0';


        if(i == 0) {
                printf("Somehow couldn't find first online cpu!!\n");
                printf("Assuming SMT=1\n");
                fflush(stdout);
                return smt;
        }

        printf("First online cpu = %s\n", on_cpu);


        /* Now look into thread_siblings_list file to find SMT topology */

        sprintf(fname, "/sys/devices/system/cpu/cpu%s/topology/thread_siblings_list", on_cpu);
        printf("Looking into file -- %s\n", fname);

        fp = fopen(fname, "r");

        if (fp == NULL) {
                printf("Failed to open %s file\n", fname);
                printf("errno = %d, %s\n", errno, strerror(errno));
                printf("Assuming SMT=1\n");
                return smt;
        }

        if ( fgets(smt_str, 10, fp) != NULL) {
                printf("SMT topology cpu%s = %s", on_cpu, smt_str);

				sscanf(smt_str, "%d-%d", &start_cpu, &end_cpu);

				if(end_cpu >= start_cpu)
					smt = (end_cpu-start_cpu) + 1;
		}

		fclose(fp);

        printf("SMT = %d\n", smt);
        fflush(stdout);

		return smt;

}
int update_syscfg(void)
{
	int lock1,lock2,lock3,lock4,lock5,lock6,lock7,lock8,lock9,lock10,lock11,lock12,lock13,lock14,lock18,lock19;
    int r1,r2,r3,r4,r5,r6,r7,r8,r9,r10,r11,r12,r13,r14,r15,r16,r17,r18,r19,rc;
	int i,j;
    unsigned int pvr;

    int temp_shm_key;
	temp_shm_key = 0;
	lock1=lock2=lock3=lock4=lock5=lock6=lock7=lock8=lock9=lock10=lock11=lock12=lock13=lock14=lock18=lock19=0;
	r1=r2=r3=r4=r5=r6=r7=r8=r9=r10=r11=r12=r13=r14=r15=r16=r17=r18=r19=rc=0;
	i=j=0;

	read_core_exclusion_array();

	if ( global_ptr == NULL ) {
	/* Check if shm is already created or not*/
		temp_shm_key = shmget(SYSCFG_SHM_KEY,(sizeof(GLOBAL_SYSCFG)),(IPC_CREAT | IPC_EXCL | S_IRWXU| S_IRWXG | S_IRWXO));
		if ( temp_shm_key == -1 && errno == EEXIST ) {
			rc = attach_to_syscfg();
			if(rc){
					sprintf(msg,"attach_to_syscfg failed inside update_syscfg with errno=%d\n",rc);
					hxfmsg(misc_htx_data, 0, HTX_HE_INFO, msg);
				return(rc);
			}
        }
		else if ( temp_shm_key != -1 ) {
                sprintf(msg,"Error in shmget when global_ptr is NULL. errno=%d\n", errno);
                hxfmsg(misc_htx_data, 0, HTX_HE_INFO, msg);
            return(-1);
		}
		else {
				sprintf(msg,"Error in shmget when global_ptr is NULL. errno=%d\n", errno);
				hxfmsg(misc_htx_data, 0, HTX_HE_INFO, msg);
			return(-1);
	    }
	}

	pvr = get_true_cpu_version();
	pvr = pvr>>16;
	global_ptr->global_pvr=pvr;

	/* copying the excluded cores array to shm */
    for(i=0; i<MAX_CORES_PER_CHIP; i++){
        for(j=0;j<3;j++){
			global_ptr->syscfg.numberArray[i][j]=numberArray_copy[i][j];
        }
    }


	/* *******************************lpar details update ******************************* */
	lock1=pthread_rwlock_wrlock(&(global_ptr->global_lpar.rw));

	if (lock1!=0  ) {
			sprintf(msg,"global_ptr->global_lpar.rw_lock() failed with rc=%d\n", lock1);
			hxfmsg(misc_htx_data, -1, HTX_HE_HARD_ERROR, msg);
			return(lock1);
    }

	r1=phy_logical_virt_cpus_update();
	if ( r1 ) {
			sprintf(msg,"phy_logical_virt_cpus_update() failed with rc=%d\n", r1);
			hxfmsg(misc_htx_data, 0, HTX_HE_INFO, msg);
    }

	r2=get_env_details_update();
	if ( r2 ) {
			sprintf(msg,"get_env_details_update() failed with rc=%d\n", r2);
			hxfmsg(misc_htx_data, 0, HTX_HE_INFO, msg);
    }

	r3=get_lpar_details_update();
	if ( r3 ) {
			sprintf(msg,"get_lpar_details__update() failed with rc=%d\n", r3);
			hxfmsg(misc_htx_data, 0, HTX_HE_INFO, msg);
    }
	lock2=pthread_rwlock_unlock(&(global_ptr->global_lpar.rw));
	if (lock2 != 0  ) {
			sprintf(msg,"global_ptr->global_lpar.rw_unlock() failed with rc= %d ", lock2);
			hxfmsg(misc_htx_data, -1, HTX_HE_HARD_ERROR, msg);
			return(lock2);
	}


	/* *******************************memory details update ******************************* */

	lock3=pthread_rwlock_wrlock(&(global_ptr->global_memory.rw));

	if (lock3!=0  ) {
			sprintf(msg,"global_ptr->global_memory.rw_lock3() failed with rc=%d\n", lock3);;
			hxfmsg(misc_htx_data, -1, HTX_HE_HARD_ERROR, msg);
			return(lock3);
    }

	r4=get_memory_size_update();
	if ( r4 ) {
			sprintf(msg,"get_memory_size__update() failed with rc=%d\n", r4);
			hxfmsg(misc_htx_data, 0, HTX_HE_INFO, msg);
    }

    r5=get_page_size_update();
    if ( r5 ) {
			sprintf(msg,"get_page_size__update() failed with rc=%d\n", r5);
			hxfmsg(misc_htx_data, 0, HTX_HE_INFO, msg);
    }

	r18 = get_page_details();
    if ( r18 ) {
            sprintf(msg,"get_page_details() failed with rc=%d\n", r18);
            hxfmsg(misc_htx_data, 0, HTX_HE_INFO, msg);
    }

    r19 = get_memory_pools();
    if(r19 > 0) {
		global_ptr->global_memory.num_numa_nodes = r19;
	}
	else{
		sprintf(msg,"global_memory.num_numa_nodes giving value of %d \n",global_ptr->global_memory.num_numa_nodes);
		hxfmsg(misc_htx_data, -1, HTX_HE_SOFT_ERROR, msg);
	}

/*	r7=get_memory_details_update();
	if ( r7 ) {
			sprintf(msg,"get_memory_details_update() failed with rc=%d\n", r7);
			hxfmsg(misc_htx_data, 0, HTX_HE_INFO, msg);
    } */

	lock4=pthread_rwlock_unlock(&(global_ptr->global_memory.rw));
	if (lock4!=0  ) {
			sprintf(msg,"global_ptr->global_memory.rw_unlock() failed with rc=%d\n", lock4);
			hxfmsg(misc_htx_data, -1, HTX_HE_HARD_ERROR, msg);
			return(lock4);
    }


	/* *******************************cache details update ******************************* */
	lock5=pthread_rwlock_wrlock(&(global_ptr->global_cache.rw));

	if (lock5!=0  ) {
			sprintf(msg,"global_ptr->global_cache.rw_unlock() failed with rc=%d\n", lock5);
			hxfmsg(misc_htx_data, -1, HTX_HE_HARD_ERROR, msg);
			return(lock5);
    }
	r8=L1cache_update();
	if ( r8 ) {
			sprintf(msg,"L1cache_update() failed with rc = %d \n", r8);
			hxfmsg(misc_htx_data, 0, HTX_HE_INFO, msg);
    }
	r9=L2L3cache_update();
	if ( r9 ) {
			sprintf(msg,"L2cache_update() failed with rc=%d\n", r9);
			hxfmsg(misc_htx_data, 0, HTX_HE_INFO, msg);
    }

	lock6 = pthread_rwlock_unlock(&(global_ptr->global_cache.rw));
	if (lock6!=0  ) {
			sprintf(msg,"global_ptr->global_cache.rw_unlock() failed with rc=%d\n", lock6);
			hxfmsg(misc_htx_data, -1, HTX_HE_HARD_ERROR, msg);
			return(lock6);
	}

	/* *******************************hardware config update ******************************* */

	lock7 = pthread_rwlock_wrlock(&(global_ptr->syscfg.rw));

	if (lock7!=0  ) {
			sprintf(msg,"global_ptr->global_syscfg.rw_lock() failed with rc=%d\n", lock7);
			hxfmsg(misc_htx_data, -1, HTX_HE_HARD_ERROR, msg);
			return(lock7);
    }
	lock18 = pthread_rwlock_wrlock(&(global_ptr->system_cpu_map.stat.rw));

	if (lock18!=0  ) {
			sprintf(msg,"global_ptr->system_cpu_map.rw_lock() inside get_hardware_config_update failed with rc=%d\n", lock18);
			hxfmsg(misc_htx_data, -1, HTX_HE_HARD_ERROR, msg);
			return(lock18);
    }

	r11=get_hardware_config_update();

	if ( r11 ) {
			sprintf(msg,"get_hardware_config_update() failed with rc=%d\n", r11);
			hxfmsg(misc_htx_data, 0, HTX_HE_INFO, msg);
    }
	lock19= pthread_rwlock_unlock(&(global_ptr->system_cpu_map.stat.rw));
    if (lock19!=0  ) {
			sprintf(msg,"global_ptr->system_cpu_map.rw_unlock() failed with rc=%d\n", lock19);
			hxfmsg(misc_htx_data, -1, HTX_HE_HARD_ERROR, msg);
			return(lock19);
    }

	lock8 = pthread_rwlock_unlock(&(global_ptr->syscfg.rw));
	if (lock8!=0  ) {
			sprintf(msg,"global_ptr->syscfg.rw_unlock() failed with rc=%d\n", lock8);
			hxfmsg(misc_htx_data, -1, HTX_HE_HARD_ERROR, msg);
			return(lock8);
	}
	/* *******************************hardware stat details update ************************* */

	lock9 = pthread_rwlock_wrlock(&(global_ptr->system_cpu_map.stat.rw));

	if (lock9!=0  ) {
			sprintf(msg,"global_ptr->system_cpu_map.stat.rw_lock() failed with rc=%d\n", lock9);
			hxfmsg(misc_htx_data, -1, HTX_HE_HARD_ERROR, msg);
			return(lock9);
    }

	r12=get_hardware_stat_update();
	if ( r12) {
			sprintf(msg,"get_hardware_stat_update() failed with rc=%d\n", r12);
			hxfmsg(misc_htx_data, 0, HTX_HE_INFO, msg);
     }
	lock12= pthread_rwlock_unlock(&(global_ptr->system_cpu_map.stat.rw));
	if (lock12!=0  ) {
			sprintf(msg,"global_ptr->system_cpu_map.stat.rw_unlock() failed with rc=%d\n", lock12);
			hxfmsg(misc_htx_data, -1, HTX_HE_HARD_ERROR, msg);
			return(lock12);
	}

	/* *******************************core details update ********************************** */

	lock13 = pthread_rwlock_wrlock(&(global_ptr->global_core.rw));

	if (lock13!=0  ) {
			sprintf(msg,"global_ptr->global_core.rw_lock() failed with rc=%d\n", lock13);
			hxfmsg(misc_htx_data, -1, HTX_HE_HARD_ERROR, msg);
			return(lock13);
    }
	r17 = get_hw_smt();
	if ( r17) {
			sprintf(msg,"get_hw_smt() failed with rc=%d\n", r17);
			hxfmsg(misc_htx_data, 0, HTX_HE_INFO, msg);
    }

	r15 = get_min_smt();
	if ( r15) {
			sprintf(msg,"get_min_smt() failed with rc=%d\n", r15);
			hxfmsg(misc_htx_data, 0, HTX_HE_INFO, msg);
    }

	r16 = get_max_smt();
	if ( r16) {
			sprintf(msg,"get_max_smt() failed with rc=%d\n", r16);
			hxfmsg(misc_htx_data, 0, HTX_HE_INFO, msg);
    }


	r13=get_smt_details_update();
	if ( r13) {
			sprintf(msg,"get_smt_details_update() failed with rc=%d\n", r13);
			hxfmsg(misc_htx_data, 0, HTX_HE_INFO, msg);
    }

	r14=get_core_details_update();
	if ( r14) {
			sprintf(msg,"get_core_details_update() failed with rc=%d\n", r14);
			hxfmsg(misc_htx_data, 0, HTX_HE_INFO, msg);
    }
	lock14 = pthread_rwlock_unlock(&(global_ptr->global_core.rw));
	if (lock14!=0  ) {
			sprintf(msg,"global_ptr->global_core.rw_unlock() failed with rc=%d\n", lock14);
			hxfmsg(misc_htx_data, -1, HTX_HE_HARD_ERROR, msg);
			return(lock14);
	}

    return (r1|r2|r3|r5|r6|r7|r8|r9|r10|r11|r12|r13|r14|r15|r16|r17);
}

/**********************************************************************************************/
/*   Function to parse the resource string to provide resources enabled in the given string   */
/**********************************************************************************************/
int parse_filter(char filter_str[][MAX_FILTER_STR], struct resource_filter_info *filter, int num_cpu_filters)
{
    int i= 0, j = 0, k = 0, filt_idx, resource_array[24];
    int num_nodes = 0, num_chips = 0, num_cores=0, num_cpus=0;
    unsigned long node_num, chip_num, core_num, cpu_num;
    unsigned int begin_node, begin_chip, begin_core, begin_cpu;
    unsigned int end_node, end_chip, end_core, end_cpu;
    char tmp_str[128], tmp_str1[128], msg[8092];
    char *chr_ptr[4], *end_ptr, htx_msg[256];
    char *ptr[16], *tmp, tmp_filter_str[128];

    filter->num_nodes =0;
    for (node_num = 0; node_num < MAX_NODES; node_num++ ) {
        for (chip_num = 0; chip_num < MAX_CHIPS_PER_NODE; chip_num++) {
            for (core_num = 0; core_num < MAX_CORES_PER_CHIP; core_num++) {
                for (cpu_num = 0; cpu_num < MAX_CPUS_PER_CORE; cpu_num++) {
                    filter->node[node_num].chip[chip_num].core[core_num].lprocs[cpu_num]=-1;
                }
                filter->node[node_num].chip[chip_num].core[core_num].num_procs = 0;
                filter->node[node_num].chip[chip_num].core[core_num].core_num = -1;
            }
            filter->node[node_num].chip[chip_num].num_cores = 0;
            filter->node[node_num].chip[chip_num].chip_num = -1;
        }
        filter->node[node_num].num_chips = 0;
        filter->node[node_num].node_num = -1;
    }

    for (filt_idx = 0; filt_idx < num_cpu_filters; filt_idx++) {
        strcpy(tmp_filter_str, filter_str[filt_idx]);
        if ((chr_ptr[i] = strtok(tmp_filter_str, "NPCT")) != NULL) {
            i++;
            while ((chr_ptr[i] = strtok(NULL, "NPCT")) != NULL) {
                i++;
            }
        } else {
            sprintf(htx_msg, "[%d]%s:Improper value is provided for cpu filter: %s, filter num:%d\n",
                    __LINE__, __FUNCTION__, filter_str[filt_idx], filt_idx);
            hxfmsg(misc_htx_data, 0, HTX_HE_INFO, htx_msg);
            return (-1);
        }
        filter->num_nodes = get_num_of_nodes_in_sys();
        if (chr_ptr[0][0] == '*') {
           for (node_num = 0; node_num < filter->num_nodes; node_num++) {
               if (filter->node[node_num].node_num == -1) {
                   filter->node[node_num].node_num = node_num;
               }
           }
        } else if (strchr(chr_ptr[0], '[') != NULL) {  /* indicates single/multiple values are given in [] */
            strcpy(tmp_str, chr_ptr[0]);
            if ((ptr[j++] = strtok(tmp_str,"[],")) == NULL) {
                sprintf(htx_msg, "[%d]%s: Improper value is provided for cpu filter: %s, filter num:%d\n",
                        __LINE__, __FUNCTION__, filter_str[filt_idx], filt_idx);
                hxfmsg(misc_htx_data, 0, HTX_HE_INFO, htx_msg);
                return (-1);
            }
            while ((ptr[j]=strtok(NULL,",]")) != NULL ) {
                j++;
            }
            for (k = 0; k < j; k++) {
                if ((tmp = strchr(ptr[k],'%')) != NULL) { /* indicates a percentage of nodes is given in filter->*/
                    num_nodes = (filter->num_nodes * (float)((atoi(tmp+1))/100.0));
                    if (num_nodes < 0 || num_nodes > filter->num_nodes) {
                        sprintf (htx_msg, "[%d]%s: Improper value is provided for cpu filter: %s, filter num:%d\n \
                                 node percentage as %d(num_nodes=%d)\n", __LINE__, __FUNCTION__, filter_str[filt_idx],
                                 filt_idx, atoi(tmp+1), filter->num_nodes);
                        hxfmsg(misc_htx_data, 0, HTX_HE_INFO, htx_msg);
                        return (-1);
                    }
                    if (num_nodes == 0) {
                        num_nodes = 1;
                    }
                    for (node_num = 0; node_num < filter->num_nodes; node_num++) {
                        if (filter->node[node_num].node_num == -1) {
                            filter->node[node_num].node_num = node_num;
                        }
                    }
                } else if (strchr(ptr[k], '-') != NULL) { /* indicates that range of nodes is given in filter->*/
                    strcpy (tmp_str1, ptr[k]);
                    if ((tmp = strtok (tmp_str1, "-")) == NULL) {
                        sprintf (htx_msg, "[%d]%s: Improper value is provided for cpu_filer:%s,filter num:%d\n", \
                                 __LINE__, __FUNCTION__, filter_str[filt_idx], filt_idx);
                        hxfmsg(misc_htx_data, 0, HTX_HE_INFO, htx_msg);
                        return(-1);
                    }
                    errno = 0;
                    begin_node = strtol (tmp, &end_ptr, 10);
                    if (begin_node >= filter->num_nodes ||  errno != 0 || end_ptr == ptr[k]) {
                        sprintf (htx_msg, "[%d]%s: Improper value is provided for cpu_filer:%s,filter num:%d\n \
                                 node field as %d ( must be < %d)\n", __LINE__, __FUNCTION__, filter_str[filt_idx], \
                                 filt_idx, begin_node, filter->num_nodes);
                         hxfmsg(misc_htx_data, 0, HTX_HE_INFO, htx_msg);
                         return (-1);
                    }
                    if ((tmp = strtok (NULL, "-")) == NULL) {
                        hxfmsg(misc_htx_data, 0, HTX_HE_INFO, msg);
                    }
                    errno = 0;
                    if (tmp[0] == 'n' || tmp[0] == 'N') {
                        end_node = filter->num_nodes - 1;
                    } else {
                        end_node = strtol (tmp, &end_ptr, 10);
                    }
                    if (end_node >= filter->num_nodes ||  errno != 0 || end_ptr == ptr[k]) {
                        sprintf (htx_msg, "[%d]%s: Improper value is provided for cpu_filer:%s,filter num:%d\n \
                                 node field as %d ( must be < %d)\n", __LINE__, __FUNCTION__, filter_str[filt_idx],
                                 filt_idx, end_node, filter->num_nodes);
                        hxfmsg(misc_htx_data, 0, HTX_HE_INFO, htx_msg);
                        return (-1);
                    }
                    for (node_num = begin_node; node_num <= end_node; node_num++) {
                        if (filter->node[node_num].node_num == -1) {
                            filter->node[node_num].node_num = node_num;
                        }
                    }
                } else {
                    errno = 0;
                    node_num = strtol (ptr[k], &end_ptr, 10);
                    if (node_num >= filter->num_nodes || errno != 0 || end_ptr == ptr[k]) {
                        sprintf (htx_msg, "[%d]%s: Improper value is provided for cpu_filer:%s,filter num:%d\n \
                                 node field as %lu ( must be < %d)\n", __LINE__, __FUNCTION__, filter_str[filt_idx],
                                 filt_idx, node_num, filter->num_nodes);
                        hxfmsg (misc_htx_data, 0, HTX_HE_INFO, htx_msg);
                        return(-1);
                    } else {
                        if (filter->node[node_num].node_num == -1) {
                            filter->node[node_num].node_num = node_num;
                        }
                    }
                }
            }
        } else if ((tmp = strchr (chr_ptr[0], '%')) != NULL) { /* indicates a percentage of nodes is given in filter->*/
            num_nodes = (filter->num_nodes * (float)((atoi(tmp+1))/100.0));
            if ((num_nodes < 0) ||  (num_nodes > filter->num_nodes)) {
                sprintf (htx_msg, "[%d]%s: Improper value is provided for cpu_filer:%s,filter num:%d, \
                         node percentage as %d (num_nodes=%d)\n",   __LINE__, __FUNCTION__, filter_str[filt_idx], 
                         filt_idx, num_nodes, filter->num_nodes);
                hxfmsg (misc_htx_data, 0, HTX_HE_INFO, msg);
                return (-1);
            }
            if (num_nodes == 0) {
                num_nodes = 1;
            }
            for (node_num = 0; node_num < num_nodes; node_num++) {
                if (filter->node[node_num].node_num == -1) {
                    filter->node[node_num].node_num = node_num;
                }
            }
        } else {
            errno = 0;
            node_num = strtol (chr_ptr[0], &end_ptr, 10);
            if (node_num >= filter->num_nodes ||  errno != 0 || end_ptr == chr_ptr[0]) {
                 sprintf (htx_msg, "[%d]%s: Improper value is provided for cpu_filer:%s,filter num:%d, \
                         node field as %lu ( must be < %d)\n", __LINE__, __FUNCTION__, filter_str[filt_idx],
                         filt_idx, node_num, filter->num_nodes);
                hxfmsg (misc_htx_data, 0, HTX_HE_INFO, msg);
                return (-1);
            }
            if (filter->node[node_num].node_num == -1) {
                filter->node[node_num].node_num = node_num;
            }
        }
        /* Parse for chip detail for each enabled node */
        for (node_num = 0; node_num < filter->num_nodes; node_num++) {
            if (filter->node[node_num].node_num == -1 || filter->node[node_num].num_chips != 0) {
                continue;
            }
            filter->node[node_num].num_chips = get_chips_in_node (node_num, resource_array);
            if (chr_ptr[1][0] == '*') {
                for (chip_num = 0; chip_num < filter->node[node_num].num_chips; chip_num++) {
                    if (filter->node[node_num].chip[chip_num].chip_num == -1) {
                        filter->node[node_num].chip[chip_num].chip_num = resource_array[chip_num];
                    }
                }
            } else if (strchr(chr_ptr[1], '[') != NULL) { /* indicates single/multiple values are given in [] */
                strcpy(tmp_str, chr_ptr[1]);
                j = 0;
                if ((ptr[j++] = strtok(tmp_str,"[],")) == NULL){
                    sprintf (htx_msg, "[%d]%s: Improper value is provided for cpu_filer:%s,filter num:%d\n",
                             __LINE__, __FUNCTION__, filter_str[filt_idx], filt_idx);
                    hxfmsg(misc_htx_data, 0, HTX_HE_INFO, htx_msg);
                    return (-1);
                }
                while ((ptr[j] = strtok(NULL, ",]")) != NULL ) {
                    j++;
                }
                for (k = 0; k < j; k++) {
                    if ((tmp = strchr (ptr[k], '%')) != NULL) { /* indicates a percentage of chips is given in filter->*/
                        num_chips = (filter->node[node_num].num_chips * (float)((atoi (tmp+1)) / 100.0));
                        if ((num_chips < 0) ||  (num_chips > filter->node[node_num].num_chips)) {
                            sprintf (htx_msg, "[%d]%s: Improper value is provided for cpu_filer:%s,filter num:%d\n \
                                     chip percentage as %d(num_chips=%d)\n", __LINE__, __FUNCTION__, \
                                     filter_str[filt_idx], filt_idx, atoi(tmp+1), filter->node[node_num].num_chips);
                            hxfmsg (misc_htx_data, 0, HTX_HE_INFO, htx_msg);
                            return (-1);
                        }
                        if (num_chips == 0) {
                            num_chips = 1;
                        }
                        for (chip_num = 0; chip_num < num_chips; chip_num++) {
                            if (filter->node[node_num].chip[chip_num].chip_num == -1) {
                                filter->node[node_num].chip[chip_num].chip_num = resource_array[chip_num];
                            }
                        }
                    } else if (strchr (ptr[k], '-') != NULL) { /* indicates that a range of chips is given in filter->*/
                        strcpy(tmp_str1, ptr[k]);
                        if ((tmp = strtok (tmp_str1, "-")) == NULL) {
                            sprintf (htx_msg, "[%d]%s: Improper value is provided for cpu_filer:%s,filter num:%d\n",
                                    __LINE__, __FUNCTION__, filter_str[filt_idx], filt_idx);
                            hxfmsg(misc_htx_data, 0, HTX_HE_INFO, htx_msg);
                            return (-1);
                        }
                        errno = 0;
                        begin_chip = strtol (tmp, &end_ptr, 10);
                        if (begin_chip >= filter->node[node_num].num_chips || errno != 0 || end_ptr == ptr[k]) {
                            sprintf (htx_msg, "[%d]%s:Improper value is provided for cpu_filer:%s,filter num:%d\n \
                                     chip field as %d ( must be < %d)\n", __LINE__, __FUNCTION__, filter_str[filt_idx],
                                     filt_idx, begin_chip, filter->node[node_num].num_chips);
                            hxfmsg(misc_htx_data, 0, HTX_HE_INFO, htx_msg);
                            return (-1);
                        }
                        if ((tmp = strtok (NULL, "-")) == NULL) {
                            sprintf (htx_msg, "[%d]%s: Improper value is provided for cpu_filer:%s,filter num:%d\n",
                                    __LINE__, __FUNCTION__, filter_str[filt_idx], filt_idx);
                            hxfmsg(misc_htx_data, 0, HTX_HE_INFO, htx_msg);
                            return (-1);
                        }
                        errno = 0;
                        if (tmp[0] == 'n' || tmp[0] == 'N') {
                            end_chip = filter->node[node_num].num_chips - 1;
                        } else {
                            end_chip = strtol (tmp, &end_ptr, 10);
                        }
                        if (end_chip >= filter->node[node_num].num_chips || errno != 0 || end_ptr == ptr[k]) {
                            sprintf (htx_msg, "[%d]%s:Improper value is provided for cpu_filer:%s,filter num:%d\n \
                                     chip field as %d ( must be < %d)\n", __LINE__, __FUNCTION__, filter_str[filt_idx],
                                     filt_idx, end_chip, filter->node[node_num].num_chips);
                            hxfmsg(misc_htx_data, 0, HTX_HE_INFO, htx_msg);
                            return (-1);
                        }
                        for (chip_num = begin_chip; chip_num <= end_chip; chip_num++) {
                            if (filter->node[node_num].chip[chip_num].chip_num == -1) {
                                filter->node[node_num].chip[chip_num].chip_num = resource_array[chip_num];
                            }
                        }
                    } else {
                        errno = 0;
                        chip_num = strtol (ptr[k], &end_ptr, 10);
                        if (chip_num > filter->node[node_num].num_chips || errno != 0 || end_ptr == ptr[k]) {
                            hxfmsg(misc_htx_data, 0, HTX_HE_INFO, msg);
                        }
                        if (filter->node[node_num].chip[chip_num].chip_num == -1) {
                            filter->node[node_num].chip[chip_num].chip_num = resource_array[chip_num];
                        }
                    }
                }
            } else if ((tmp = strchr (chr_ptr[1], '%')) != NULL) { /* indicates a percentage of chips is given in filter->*/
                num_chips = (filter->node[node_num].num_chips * (float)(( atoi(tmp+1))/100.0));
                if ((num_chips < 0) ||  (num_chips > filter->node[node_num].num_chips)) {
                    sprintf (htx_msg, "[%d]%s:Improper value is provided for cpu_filer:%s,filter num:%d\n \
							chip percentage as %d(num_chips=%d)\n", __LINE__, __FUNCTION__, filter_str[filt_idx],
                             filt_idx, atoi(tmp+1), filter->node[node_num].num_chips);
                    hxfmsg(misc_htx_data, 0, HTX_HE_INFO, htx_msg);
                    return (-1);
                }
                if (num_chips == 0) {
                    num_chips = 1;
                }
                for (chip_num = 0; chip_num < num_chips; chip_num++) {
                    if (filter->node[node_num].chip[chip_num].chip_num == -1) {
                        filter->node[node_num].chip[chip_num].chip_num = resource_array[chip_num];
                    }
                }
            } else {
                errno = 0;
                chip_num = strtol (chr_ptr[1], &end_ptr, 10);
                if (chip_num >= filter->node[node_num].num_chips ||  errno != 0 || end_ptr == chr_ptr[1]) {
                    sprintf (htx_msg, "[%d]%s:Improper value is provided for cpu_filer:%s,filter num:%d\n \
                             chip field as %lu ( must be < %d)\n", __LINE__, __FUNCTION__, filter_str[filt_idx],
                             filt_idx, chip_num, filter->node[node_num].num_chips);
                    hxfmsg(misc_htx_data, 0, HTX_HE_INFO, msg);
                    return (-1);
                }
                if (filter->node[node_num].chip[chip_num].chip_num == -1) {
                    filter->node[node_num].chip[chip_num].chip_num = resource_array[chip_num];
                }
            }

            /* Start parsing for core string */
            for (chip_num = 0; chip_num < filter->node[node_num].num_chips; chip_num++) {
                if (filter->node[node_num].chip[chip_num].chip_num  == -1 || filter->node[node_num].chip[chip_num].num_cores != 0) {
                    continue;
                }
                filter->node[node_num].chip[chip_num].num_cores = get_cores_in_chip (node_num, chip_num, resource_array);
                if (chr_ptr[2][0] == '*') {
                    for (core_num = 0; core_num < filter->node[node_num].chip[chip_num].num_cores; core_num++) {
                        if (filter->node[node_num].chip[chip_num].core[core_num].core_num == -1) {
                            filter->node[node_num].chip[chip_num].core[core_num].core_num = resource_array[core_num];
                        }
                    }
                } else if (strchr (chr_ptr[2], '[') != NULL) { /*indicates single/multiple values are given in [] */
                    j = 0;
                    strcpy (tmp_str, chr_ptr[2]);
                    if ((ptr[j++] = strtok (tmp_str, "[],")) == NULL) {
                        sprintf (htx_msg, "[%d]%s:Improper value is provided for cpu_filer:%s,filter num:%d\n",
                                 __LINE__, __FUNCTION__, filter_str[filt_idx], filt_idx);
                        hxfmsg(misc_htx_data, 0, HTX_HE_INFO, htx_msg);
                        return (-1);
                    }
                    while ((ptr[j] = strtok (NULL, ",]")) != NULL) {
                        j++;
                   }
                   for (k = 0; k < j; k++) {
                       if ((tmp = strchr (ptr[k], '%')) != NULL) { /* indicates a percentage of cores is given in filter->*/
                           num_cores = (filter->node[node_num].chip[chip_num].num_cores * (float)(( atoi(tmp+1))/100.0));
                           if ((num_cores < 0) ||  (num_cores > filter->node[node_num].chip[chip_num].num_cores)) {
                               sprintf (htx_msg, "[%d]%s:Improper value is provided for cpu_filer:%s,filter num:%d, \
                                       core percentage as %d(num_cores=%d)\n",  __LINE__, __FUNCTION__, filter_str[filt_idx],
                                       filt_idx, atoi(tmp+1), filter->node[node_num].chip[chip_num].num_cores);
                               hxfmsg(misc_htx_data, 0, HTX_HE_INFO, htx_msg);
                               return (-1);
                           }
                           if (filter->node[node_num].chip[chip_num].core[core_num].core_num == -1) {
                               filter->node[node_num].chip[chip_num].core[core_num].core_num = resource_array[core_num];
                           }
                       } else if (strchr (ptr[k], '-') != NULL) { /* indicates a range of cores given in filter->*/
                           strcpy (tmp_str1, ptr[k]);
                           if ((tmp = strtok (tmp_str1, "-")) == NULL) {
                               sprintf (htx_msg, "[%d]%s:Improper value is provided for cpu_filer:%s,filter num:%d",
                               __LINE__, __FUNCTION__, filter_str[filt_idx], filt_idx);
                               hxfmsg(misc_htx_data, 0, HTX_HE_INFO, htx_msg);
                               return (-1);
                           }
                           errno = 0;
                           begin_core = strtol (tmp, &end_ptr, 10);
                           if (begin_core >= filter->node[node_num].chip[chip_num].num_cores || errno != 0 || end_ptr == ptr[k]) {
                               sprintf (htx_msg, "[%d]%s:Improper value is provided for cpu_filer:%s,filter num:%d, \
                                       core field as %d ( must be < %d)\n", __LINE__, __FUNCTION__, filter_str[filt_idx],
                                       filt_idx, begin_core, filter->node[node_num].chip[chip_num].num_cores);
                               hxfmsg(misc_htx_data, 0, HTX_HE_INFO, htx_msg);
                               return (-1);
                           }
                           if ((tmp = strtok (NULL, "-")) == NULL) {
                               hxfmsg(misc_htx_data, 0, HTX_HE_INFO, msg);
                           }
                           errno = 0;
                           if (tmp[0] == 'n' || tmp[0] == 'N') {
                               end_core = filter->node[node_num].chip[chip_num].num_cores - 1;
                           } else {
							   end_core = strtol (tmp, &end_ptr, 10);
                           }
                           if (end_core >= filter->node[node_num].chip[chip_num].num_cores || errno != 0 || end_ptr == ptr[k]) {
                               sprintf (htx_msg, "[%d]%s:Improper value is provided for cpu_filer:%s,filter num:%d, \
                                       core field as %d ( must be < %d)\n", __LINE__, __FUNCTION__, filter_str[filt_idx],
                                       filt_idx, end_core, filter->node[node_num].chip[chip_num].num_cores);
                               hxfmsg(misc_htx_data, 0, HTX_HE_INFO, htx_msg);
                               return (-1);
                           }
                           for (core_num = begin_core; core_num <= end_core; core_num++) {
                               if (filter->node[node_num].chip[chip_num].core[core_num].core_num == -1) {
                                   filter->node[node_num].chip[chip_num].core[core_num].core_num = resource_array[core_num];
                               }
                           }
                       } else {
                           errno = 0;
                           core_num = strtol (ptr[k], &end_ptr, 10);
                           if (core_num >= filter->node[node_num].chip[chip_num].num_cores ||  errno != 0 || end_ptr == ptr[k]) {
                               sprintf (htx_msg, "[%d]%s:Improper value is provided for cpu_filer:%s,filter num:%d, \
                                        core filed as %lu ( must be < %d)\n", __LINE__, __FUNCTION__, filter_str[filt_idx],
                                        filt_idx, core_num, filter->node[node_num].chip[chip_num].num_cores);
                               hxfmsg(misc_htx_data, 0, HTX_HE_INFO, htx_msg);
                               return (-1);
                           }
                           if (filter->node[node_num].chip[chip_num].core[core_num].core_num == -1) {
                               filter->node[node_num].chip[chip_num].core[core_num].core_num = resource_array[core_num];
                           }
                       }
                    }
                } else if ((tmp = strchr (chr_ptr[2], '%')) != NULL ) {/* indicates a percentage of cores is given in filter->*/
                    num_cores = (filter->node[node_num].chip[chip_num].num_cores * (float)(( atoi(tmp+1))/100.0));
                    if ((num_cores < 0) ||  (num_cores > filter->node[node_num].chip[chip_num].num_cores)) {
                         sprintf (htx_msg, "[%d]%s:Improper value is provided for cpu_filer:%s,filter num:%d, \
                                  core filed as %d ( must be < %d)\n", __LINE__, __FUNCTION__, filter_str[filt_idx],
                                  filt_idx, atoi(tmp+1), filter->node[node_num].chip[chip_num].num_cores);
                         hxfmsg(misc_htx_data, 0, HTX_HE_INFO, htx_msg);
                         return (-1);
                    }
                    if (filter->node[node_num].chip[chip_num].core[core_num].core_num == -1) {
                        filter->node[node_num].chip[chip_num].core[core_num].core_num = resource_array[core_num];
                    }
                } else {
                    errno = 0;
                    core_num = strtol (chr_ptr[2], &end_ptr, 10);
                    if (core_num >= filter->node[node_num].chip[chip_num].num_cores ||  errno != 0 || end_ptr == ptr[k]) {
                        sprintf (htx_msg, "[%d]%s:Improper value is provided for cpu_filer:%s,filter num:%d, \
                                core field as %lu ( must be < %d)\n", __LINE__, __FUNCTION__, filter_str[filt_idx],
                                        filt_idx, core_num, filter->node[node_num].chip[chip_num].num_cores);
                        hxfmsg(misc_htx_data, 0, HTX_HE_INFO, htx_msg);
                        return (-1);
                    }
                    if (filter->node[node_num].chip[chip_num].core[core_num].core_num == -1) {
                        filter->node[node_num].chip[chip_num].core[core_num].core_num = resource_array[core_num];
                    }
                }
                /* Start parsing for cpu string */
                for (core_num = 0; core_num < filter->node[node_num].chip[chip_num].num_cores; core_num++) {
                    if ((filter->node[node_num].chip[chip_num].core[core_num].core_num == -1) || (filter->node[node_num].chip[chip_num].core[core_num].num_procs != 0)) {
                        continue;
                    }
                    filter->node[node_num].chip[chip_num].core[core_num].num_procs = get_num_of_cpus_in_core(node_num, chip_num, core_num);
                    if (chr_ptr[3][0] == '*') {
                        for (cpu_num = 0; cpu_num < filter->node[node_num].chip[chip_num].core[core_num].num_procs; cpu_num++) {
                            filter->node[node_num].chip[chip_num].core[core_num].lprocs[cpu_num] =  get_logical_cpu_num(node_num, chip_num, core_num, cpu_num);
                        }
                    } else if (strchr (chr_ptr[3], '[') != NULL) { /* Indicates single/multiple values given in [] */
                        strcpy (tmp_str, chr_ptr[3]);
                        j = 0;
                        if ((ptr[j++] = strtok (tmp_str, "[],")) == NULL) {
                            sprintf (htx_msg, "[%d]%s:Improper value is provided for cpu_filer:%s,filter num:%d\n",
                                      __LINE__, __FUNCTION__, filter_str[filt_idx], filt_idx);
                            hxfmsg(misc_htx_data, 0, HTX_HE_INFO, htx_msg);
                            return (-1);
                        }
                        while ((ptr[j] = strtok (NULL, ",]")) != NULL ) {
                            j++;
                        }
                        for (k =0; k < j; k++) {
                            if ((tmp = strchr (ptr[k], '%')) != NULL) { /* indicates a percentage of cpus is given in filter->*/
                                num_cpus = (filter->node[node_num].chip[chip_num].core[core_num].num_procs * (float) ((atoi (tmp+1)) / 100.0));
                                if ((num_cpus < 0) ||  (num_cpus > filter->node[node_num].chip[chip_num].core[core_num].num_procs)) {
                                    sprintf (htx_msg, "[%d]%s:Improper value is provided for cpu_filer:%s,filter num:%d\n"
                                             "cpu percentage as %d(num_cpus=%d)\n", __LINE__, __FUNCTION__, filter_str[filt_idx],
                                             filt_idx, atoi(tmp+1), filter->node[node_num].chip[chip_num].core[core_num].num_procs);
                                    hxfmsg(misc_htx_data, 0, HTX_HE_INFO, htx_msg);
                                    return (-1);
                                }
                                for (cpu_num = 0; cpu_num < num_cpus; cpu_num++) {
                                    if (filter->node[node_num].chip[chip_num].core[core_num].lprocs[cpu_num] == -1) {
                                        filter->node[node_num].chip[chip_num].core[core_num].lprocs[cpu_num] = get_logical_cpu_num(node_num, chip_num, core_num, cpu_num);
                                    }
                                }
                            } else if (strchr (ptr[k], '-') != NULL) {
                                strcpy (tmp_str1, ptr[k]);
                                if ((tmp = strtok (tmp_str1, "-")) == NULL) {
                                    sprintf (htx_msg, "[%d]%s:Improper value is provided  for cpu_filer:%s,filter num:%d\n",
                                             __LINE__, __FUNCTION__, filter_str[filt_idx], filt_idx);
                                    hxfmsg(misc_htx_data, 0, HTX_HE_INFO, msg);
                                    return (-1);
                                }
                                errno = 0;
                                begin_cpu = strtol (tmp, &end_ptr, 10);
                                if (begin_cpu > filter->node[node_num].chip[chip_num].core[core_num].num_procs || errno != 0 || end_ptr == ptr[k]) {
                                    sprintf (htx_msg, "[%d]%s: Improper value is provided  for cpu_filer:%s,filter num:%d\n \
                                             thread field as %d ( must be < %d)\n", __LINE__, __FUNCTION__, filter_str[filt_idx],
                                             filt_idx, begin_cpu, filter->node[node_num].chip[chip_num].core[core_num].num_procs);
                                    hxfmsg(misc_htx_data, 0, HTX_HE_INFO, msg);
                                    return (-1);
                                }
                                if ((tmp = strtok (NULL, "-")) == NULL) {
                                     sprintf (htx_msg, "[%d]%s:Improper value is provided  for cpu_filer:%s,filter num:%d\n",
                                              __LINE__, __FUNCTION__, filter_str[filt_idx], filt_idx);
                                    hxfmsg(misc_htx_data, 0, HTX_HE_INFO, msg);
                                    return (-1);
                                }
                                errno = 0;
                                if (tmp[0] == 'n' || tmp[0] == 'N') {
                                    end_cpu = filter->node[node_num].chip[chip_num].core[core_num].num_procs - 1;
                                } else {
							        end_cpu = strtol (tmp, &end_ptr, 10);
                                }
                                if (end_cpu > filter->node[node_num].chip[chip_num].core[core_num].num_procs || errno != 0 || end_ptr == ptr[k]) {
                                    sprintf (htx_msg, "[%d]%s: Improper value is provided  for cpu_filer:%s,filter num:%d\n \
                                             thread field as %d ( must be < %d)\n", __LINE__, __FUNCTION__, filter_str[filt_idx],
                                             filt_idx, end_cpu, filter->node[node_num].chip[chip_num].core[core_num].num_procs);
                                    hxfmsg(misc_htx_data, 0, HTX_HE_INFO, msg);
                                    return (-1);
                                }
                                for (cpu_num = begin_cpu; cpu_num <= end_cpu; cpu_num++) {
                                    if (filter->node[node_num].chip[chip_num].core[core_num].lprocs[cpu_num] == -1) {
                                        filter->node[node_num].chip[chip_num].core[core_num].lprocs[cpu_num] = get_logical_cpu_num(node_num, chip_num, core_num, cpu_num);
                                    }
                                }
                            } else {
                                errno = 0;
                                cpu_num = strtol (ptr[k], &end_ptr, 10);
                                if (cpu_num >= filter->node[node_num].chip[chip_num].core[core_num].num_procs  ||  errno != 0 || end_ptr == ptr[k]) {
                                    sprintf (htx_msg, "[%d]%s: Improper value is provided  for cpu_filer:%s,filter num:%d\n \
                                             thread field as %lu ( must be < %d)\n", __LINE__, __FUNCTION__, filter_str[filt_idx],
                                             filt_idx, cpu_num, filter->node[node_num].chip[chip_num].core[core_num].num_procs);
                                    hxfmsg(misc_htx_data, 0, HTX_HE_INFO, msg);
                                    return (-1);
                                }
                                if (filter->node[node_num].chip[chip_num].core[core_num].lprocs[cpu_num] == -1) {
                                    filter->node[node_num].chip[chip_num].core[core_num].lprocs[cpu_num] = get_logical_cpu_num(node_num, chip_num, core_num, cpu_num);
                                }
                            }
                        }
                    } else if ((tmp = strchr (chr_ptr[3], '%')) != NULL) { /* indicates a percentage of cpus is given in filter->*/
                        num_cpus = (filter->node[node_num].chip[chip_num].core[core_num].num_procs * (float) ((atoi (tmp+1)) / 100.0));
                        if ((num_cpus < 0) ||  (num_cpus > filter->node[node_num].chip[chip_num].core[core_num].num_procs)) {
                            sprintf (htx_msg, "[%d]%s: Improper value is provided  for cpu_filer:%s,filter num:%d\n \
                                     thread percentage as %d(num_cpus=%d)\n", __LINE__, __FUNCTION__, filter_str[filt_idx],
                                     filt_idx, atoi (tmp+1), filter->node[node_num].chip[chip_num].core[core_num].num_procs);
                            hxfmsg(misc_htx_data, 0, HTX_HE_INFO, htx_msg);
                            return (-1);
                        }
                        for (cpu_num= 0; cpu_num < num_cpus; cpu_num++) {
                            if (filter->node[node_num].chip[chip_num].core[core_num].lprocs[cpu_num] == -1) {
                                filter->node[node_num].chip[chip_num].core[core_num].lprocs[cpu_num] = get_logical_cpu_num(node_num, chip_num, core_num, cpu_num);
                            }
                        }
                    } else {
                        errno = 0;
                        cpu_num = strtol (chr_ptr[3], &end_ptr, 10);
                        if (cpu_num >= filter->node[node_num].chip[chip_num].core[core_num].num_procs  ||  errno != 0 || end_ptr == ptr[k]) {
                            sprintf (htx_msg, "[%d]%s: Improper value is provided  for cpu_filer:%s,filter num:%d\n \
                                     cpu field as %lu ( must be < %d)\n", __LINE__, __FUNCTION__, filter_str[filt_idx],
                                     filt_idx, cpu_num, filter->node[node_num].chip[chip_num].core[core_num].num_procs);
                            hxfmsg(misc_htx_data, 0, HTX_HE_INFO, htx_msg);
                            return (-1);
                        }
                        if (filter->node[node_num].chip[chip_num].core[core_num].lprocs[cpu_num] == -1) {
                            filter->node[node_num].chip[chip_num].core[core_num].lprocs[cpu_num] = get_logical_cpu_num(node_num, chip_num, core_num, cpu_num);
                        }
                    }
                } /* parsing done for cpu string */
            } /* Parsing done for core string */
        } /* Parsing done for chip string */
    }
    return 0;
}
