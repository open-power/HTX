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
/* @(#)51	1.3  src/htx/usr/lpp/htx/bin/htxd/htxd_ipc.h, htxd, htxubuntu 12/16/14 01:55:42 */



#ifndef HTXD__IPC__HEADER
#define HTXD__IPC__HEADER

#define MAX_ADDED_DEVICES 10

#define PSEUDO_EXTRA_ENTRIES 200

typedef struct
{
    char device_name[100];
    int     device_pid;
    int     ecg_shmkey;
    int     ecg_semkey;
} htxd_device_entry;


extern htxd_device_entry *	htxd_create_device_table(int);
extern int			htxd_init_sem(int, int);
extern int			htxd_release_all_semaphore(int);
extern int			htxd_cleanup_sem(int);
extern struct htxshm_hdr *	htxd_init_shm(int, int, int *);
extern int			htxd_cleanup_shm(int, void *);
extern int			htxd_create_message_queue(int);
extern void *			htxd_create_exer_table(int);
extern void *			htxd_create_system_header_info(void);
extern int			htxd_get_exer_sem_status(int, int, int *);
extern int			htxd_get_device_run_sem_status(int, int);
extern int			htxd_get_device_error_sem_status(int, int);
extern int			htxd_set_device_run_sem_status(int, int, int);
extern int			htxd_set_device_error_sem_status(int, int, int);
extern int			htxd_create_dr_sem(void);
extern int			htxd_set_dr_sem_value(int);
extern int			htxd_get_dr_sem_value(void);
extern int			htxd_get_global_activate_halt_sem_status(int);
extern int			htxd_set_global_activate_halt_sem_status(int, int);


#endif

