/* IBM_PROLOG_BEGIN_TAG */
/* 
 * Copyright 2003,2016 IBM International Business Machines Corp.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * 		 http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or
 * implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
/* IBM_PROLOG_END_TAG */
/* @(#)50	1.5  src/htx/usr/lpp/htx/bin/htxd/htxd_ipc.c, htxd, htxubuntu 2/5/15 00:48:46 */


#include <pthread.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <sys/stat.h>
#include <errno.h>

#include "htxd_ipc.h"
#include "htxd_profile.h"
#include "htxd_instance.h"
#include "htxd_ecg.h"
#include "htxd_util.h"

#include "hxiipc.h" 
/* #include <hxiipc.h> */
#include "scr_info.h"

#ifndef __HTX_LINUX__
#define SEMCTL(x,y,z,a) semctl(x,y,z,a)
#else
union semun semctl_arg;
#define SEMCTL(x,y,z,a) semctl_arg.val=a, semctl(x,y,z,semctl_arg)
#endif


int g_msgqid;
int sem_length;

htxd_device_entry   *htxd_create_device_table(int number_of_devices)
{
	htxd_device_entry *device_table = NULL;
	int shm_id = -1;


	shm_id = shmget (SHMKEY, number_of_devices * sizeof (htxd_device_entry), IPC_CREAT | S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH);
	if(shm_id == -1) {
		exit(1);
	}

	htxd_set_exer_table_shm_id(shm_id);

	device_table = (htxd_device_entry *) shmat (shm_id, (char *) 0, 0);
	if(device_table == (htxd_device_entry *)-1) {
		exit(1);
	}
	
	return device_table;
}


void * htxd_create_system_header_info(void)
{
	void *attch_address;
	int shm_id = -1;

	shm_id = shmget (REMSHMKEY, 4096, IPC_CREAT | S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH |S_IWOTH);
	if(shm_id == -1) {
		exit(1);
	}

	htxd_set_system_header_info_shm_id(shm_id);
	
	attch_address = (void *) shmat (shm_id, (char *) 0, 0);
	if(attch_address == (void *) -1) {
		exit(1);
	}

	bzero (attch_address,  4096);

	return attch_address;

}

void * htxd_create_exer_table(int number_of_exer)
{
	void *attch_address;
	int shm_id = -1;

	shm_id = shmget (SHMKEY, number_of_exer * sizeof (texer_list), IPC_CREAT | S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH);
	if(shm_id == -1) {
		exit(1);
	}

	htxd_set_exer_table_shm_id(shm_id);

	attch_address = (void *) shmat (shm_id, (char *) 0, 0);
	if(attch_address == (void *) -1) {
		exit(1);
	}
	bzero (attch_address,  number_of_exer * sizeof (texer_list));
	 
	return attch_address;
}

int htxd_create_message_queue(int message_key)
{
	int message_queue_id;

	g_msgqid = message_queue_id = msgget (message_key, IPC_CREAT | S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH); 
	if(message_queue_id == -1) {
		exit(14);
	}

	return message_queue_id;
}



/* allocating semaphore for an ECG */
int htxd_init_sem(int sem_key, int number_of_device)
{
	int sem_id;
	int i;
	int max_added_device;
	int total_number_of_device;

	max_added_device = htxd_get_max_add_device();

	total_number_of_device = number_of_device + max_added_device;

	sem_length = (total_number_of_device * SEM_PER_EXER) + SEM_GLOBAL;
	sem_id  = semget (sem_key, sem_length, IPC_CREAT | S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH);
	if(sem_id == -1) {
		return sem_id;
	}

	for (i = 0; i < (((int) (total_number_of_device) * SEM_PER_EXER) + SEM_GLOBAL); i++) {
		SEMCTL (sem_id, i, SETVAL, 0);
	}
	
	return sem_id;;
}



int htxd_cleanup_sem(int sem_id)
{
	int result;

	result = semctl(sem_id, 0, IPC_RMID, (struct semid_ds *) NULL);
	if(result == -1) {
		perror("semaphore cleanup failed");
	}

	return result;
}



int htxd_release_all_semaphore(int sem_id)
{

	union semun
	{
		int val;
		struct semid_ds *buf;
		unsigned short *array;
	}semctl_arg;
	char msg_text[256];


	semctl_arg.array = (ushort*) malloc(sem_length * sizeof(ushort) );
	if(semctl_arg.array == NULL) {
		sprintf(msg_text, "Unable to allocate memory for semctl SETVAL array.  errno = %d.", errno);
		htxd_send_message(msg_text, errno, HTX_SYS_HARD_ERROR, HTX_SYS_MSG);
		return -1;
	}

	memset(semctl_arg.array, 0, (sem_length * sizeof(ushort) ) );
	semctl(sem_id, 0, SETALL, semctl_arg);

	free(semctl_arg.array);
	semctl_arg.array = NULL;

	return 0;
}



/* allocating shared memory for an ECG */
struct htxshm_hdr * htxd_init_shm(int shm_key, int number_of_device, int *shm_id)
{
	int max_added_device;
	struct htxshm_hdr *htx_shm_ptr;

	int total_number_of_device;

	/* struct htxshm_hdr * shm_ptr; */

	int shm_size;

	max_added_device = htxd_get_max_add_device();
	/* extra devices provided to accommodate any future addition, like because of DR */
	total_number_of_device = number_of_device + max_added_device + PSEUDO_EXTRA_ENTRIES;
	shm_size = sizeof (struct htxshm_hdr) + ((total_number_of_device) * sizeof (struct htxshm_HE));

	*shm_id = shmget (shm_key, shm_size, IPC_CREAT | S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH);
	if(*shm_id == -1) {
		return NULL;
	}

	htx_shm_ptr = (struct htxshm_hdr *) shmat (*shm_id, (char *) 0, 0);
	if(htx_shm_ptr == (struct htxshm_hdr *) -1) {
		return NULL;
	}

	memset(htx_shm_ptr, 0, shm_size);
		
	return htx_shm_ptr;
}



/* cleaning up shared memory */
int htxd_cleanup_shm(int shm_id, void * htx_shm_ptr)
{
	int result;
	
	result = shmdt(htx_shm_ptr);
	if(result == -1) {
		perror("shamred memory detach failed");
	}

	result = shmctl(shm_id, IPC_RMID, NULL);
	if(result == -1) {
		perror("shamred memory remove failed");
	}

	
	return result;
}


int htxd_get_exer_sem_status(int exer_sem_id, int exer_position, int *sem_status)
{
	struct semid_ds sembuffer;

	*sem_status = 0;

	*sem_status = semctl(exer_sem_id, ( (exer_position * SEM_PER_EXER) + SEM_GLOBAL + 1), GETVAL, &sembuffer);

	return *sem_status;
} 


int htxd_set_device_run_sem_status(int device_sem_id, int device_position, int new_status)
{
	int return_code = 0;

	return_code = SEMCTL(device_sem_id, ( (device_position * SEM_PER_EXER) + SEM_GLOBAL), SETVAL, new_status);

	return return_code;
} 


int htxd_set_device_error_sem_status(int device_sem_id, int device_position, int new_status)
{
	int return_code = 0;

	return_code = SEMCTL(device_sem_id, ( (device_position * SEM_PER_EXER) + SEM_GLOBAL + 1), SETVAL, new_status);

	return return_code;
} 




int htxd_get_device_run_sem_status(int device_sem_id, int device_position)
{
	struct semid_ds device_sembuffer;
	int sem_status = 0;

	sem_status = semctl(device_sem_id, ( (device_position * SEM_PER_EXER) + SEM_GLOBAL), GETVAL, &device_sembuffer);

	return sem_status;
} 


int htxd_get_device_error_sem_status(int device_sem_id, int device_position)
{
	struct semid_ds device_sembuffer;
	int sem_status = 0;

	sem_status = semctl(device_sem_id, ( (device_position * SEM_PER_EXER) + SEM_GLOBAL + 1), GETVAL, &device_sembuffer);

	return sem_status;
} 


int htxd_get_global_activate_halt_sem_status(int sem_id)
{
	struct semid_ds device_sembuffer;
	int sem_status = -1;
	
	
	sem_status = semctl(sem_id, 0, GETVAL, &device_sembuffer);

	return sem_status;
}




int htxd_set_global_activate_halt_sem_status(int sem_id, int value)
{
	struct semid_ds device_sembuffer;
	int return_code;
	
	return_code = SEMCTL(sem_id, 0, SETVAL, value);	

	return return_code;
}



int htxd_create_dr_sem(void)
{
	int dr_sem_id = -1;
	int dr_sem_key;

	
	dr_sem_key = htxd_get_dr_sem_key();	

	dr_sem_id = semget(dr_sem_key, 1, IPC_CREAT | S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH);

	htxd_set_dr_sem_id(dr_sem_id);

	return dr_sem_id;
}


int htxd_set_dr_sem_value(int value)
{
	int return_code = 0;
	int dr_sem_id;


	dr_sem_id = htxd_get_dr_sem_id();

	return_code = SEMCTL(dr_sem_id, 0, SETVAL, value);

	return return_code;
}


int htxd_get_dr_sem_value(void)
{
	int sem_value = 0;
	struct semid_ds   dr_semids;
	int dr_sem_id;


	dr_sem_id = htxd_get_dr_sem_id();

	sem_value = semctl(dr_sem_id, 0, GETVAL, &dr_semids);

	return sem_value;
}
