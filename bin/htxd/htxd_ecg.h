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
/* @(#)43	1.4  src/htx/usr/lpp/htx/bin/htxd/htxd_ecg.h, htxd, htxubuntu 7/8/15 00:08:49 */



#ifndef HTXD__ECG__HEADER
#define HTXD__ECG__HEADER




#include "hxiipc.h"
#include "htxd_ipc.h"
#include "scr_info.h"
#include "htxd_common_define.h"


#define ECG_UNLOADED		10
#define ECG_INACTIVE		20
#define ECG_ACTIVE		30
#define ECG_PARTIALLY_RUNNING	40

#define ECG_SHMKEY_START	10601
#define ECG_SEMKEY_START	10201


typedef struct
{
	int	enable_flag;
	int	debug_flag;
	int offline_cpu;
    int time_quantum;
    int startup_time_delay;
    int log_duration;
    int pattern_length;
	char	config_file[128];
	char    sys_cpu_util[128];
}htxd_ecg_equaliser_details;


/* ecg info : contains the details of an ECG */
typedef struct htxd_ecg_info_struct
{
	struct htxd_ecg_info_struct *	ecg_info_next;
	struct htxd_ecg_info_struct *	ecg_info_previous;
	char							ecg_name[MAX_ECG_NAME_LENGTH];
	char 				ecg_start_time[26];
	int				ecg_run_start_time;
	int								ecg_status;
	char							ecg_description[20];
	int								ecg_shm_key;
	int								ecg_sem_key;
	int								ecg_shm_id;
	int								ecg_sem_id;
	int								ecg_device_count;
	union shm_pointers				ecg_shm_addr;
	int								ecg_exerciser_entries;		/* device entrie present in ecg */
	int								ecg_max_exerciser_entries;
	int								ecg_shm_exerciser_entries;	/* actual devices loaded in shm */
	htxd_ecg_equaliser_details		ecg_equaliser_info;
}htxd_ecg_info;


/* ecg manager : definition*/
typedef struct
{
	htxd_ecg_info *		ecg_info_list;
	int			loaded_device_count;
	int			ecg_device_count;
	int			ecg_list_length;
	htxd_ecg_info *		current_loading_ecg_info;
	int			message_queue_key;
	int			message_queue_id;
	int			ecg_list_shm_key;
	int			ecg_list_shm_id;
	texer_list *		exer_table;
	int			exer_table_shm_id;
	int			exer_table_length;
	tsys_hdr *		system_header_info;
	int			system_header_info_shm_id;
	char			selected_ecg_name[MAX_ECG_NAME_LENGTH];
	char			running_ecg_name[MAX_ECG_NAME_LENGTH];
}htxd_ecg_manager;


typedef struct
{
	int total_device;
	int running_device;
	int suspended_device;
	int error_device;
	int partially_running_device;
	int dr_terminated_device;
	int terminated_device;
	int died_device;
	int hung_device;
	int completed_device;
	int inactive_device;
} htxd_ecg_summary;



/* function pointer types */
typedef int(*ecg_process_function)(htxd_ecg_info*, char*);
typedef int(*ecg_device_process_function)(htxd_ecg_info*, char *, char*);


/* function prototypes for ECG */
extern int			htxd_process_all_active_ecg(ecg_process_function, char *);
extern int			htxd_process_all_active_ecg_device(ecg_device_process_function, char *, char *);
extern htxd_ecg_manager *	create_ecg_manager(void);
extern int			htxd_init_ecg_info(htxd_ecg_manager *, char *);
extern int			add_pid_to_shm(int , struct htxshm_HE *);
extern int			htxd_get_ecg_list_length(htxd_ecg_manager *);
extern htxd_ecg_info *		htxd_get_ecg_info_list(htxd_ecg_manager *);
/*extern void				htxd_remove_ecg_info_node(htxd_ecg_info *); */
extern void			htxd_get_active_ecg_name_list(htxd_ecg_manager *, char *);
extern void			htxd_remove_ecg_info_node(htxd_ecg_manager *, htxd_ecg_info *);
extern htxd_ecg_info *		htxd_get_ecg_info_node(htxd_ecg_manager *, char *);
extern int			htxd_get_exer_position_in_exer_table_by_pid(texer_list *, pid_t);
extern int			htxd_get_exer_position_in_exer_table_by_exer_name(texer_list *, char *);
extern int			htxd_get_next_key_offset(htxd_ecg_manager *);
extern int			htxd_update_exer_pid_in_exer_list(texer_list *, char *, pid_t);
extern void			htxd_display_ecg_info_list(void);
extern void			htxd_set_system_header_info_shm_with_current_shm_key(int);
extern void			htxd_set_system_header_info_shm_with_max_exer_entries(htxd_ecg_manager *,int);
extern int			htxd_get_running_ecg_count(void);
extern char *			htxd_get_running_ecg_name(void);
extern int			htxd_set_shm_with_exercisers_values_for_dr_restart(char *);
extern int			htxd_get_running_ecg_list(char *);
extern char *			htxd_get_ecg_start_time(void);
extern int			htxd_get_total_device_count(void);
extern int			htxd_get_loaded_device_count(void);

#endif



/***************************************     END : htxd_ecg.h     **********************************************/
