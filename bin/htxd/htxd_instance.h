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
/* @(#)49	1.8  src/htx/usr/lpp/htx/bin/htxd/htxd_instance.h, htxd, htxubuntu 11/24/15 23:59:23 */



#ifndef HTXD__INSTANCE__HEADER
#define HTXD__INSTANCE__HEADER

#include "htxd.h"


extern int			htxd_is_profile_initialized(htxd *);
extern htxd *			htxd_create_instance(void);
extern void			init_htxd_instance(htxd *);
extern htxd *			htxd_get_instance(void);
extern void			htxd_init_instance(htxd *);
extern int			htxd_is_daemon_idle(void);
extern int			htxd_is_daemon_selected(void);
extern void			htxd_set_daemon_idle(void);
extern int			htxd_get_daemon_state(void);
extern void			htxd_set_daemon_state(int);
extern void			htxd_set_htx_msg_pid(pid_t);
extern pid_t		htxd_get_htx_msg_pid(void);
extern void			htxd_set_htx_stats_pid(pid_t);
extern pid_t		htxd_get_htx_stats_pid(void);
extern void			htxd_set_dr_child_pid(pid_t);
extern pid_t		htxd_get_dr_child_pid(void);
extern texer_list *		htxd_get_exer_table(void);
extern int			htxd_get_exer_table_length(void);
extern int			htxd_reset_exer_pid(pid_t, char *);
extern void			htxd_display_exer_table(void);
extern int			htxd_update_command_object(char *, htxd_command *);
extern htxd_ecg_manager *	htxd_get_ecg_manager(void);
void				htxd_set_ecg_manager(htxd_ecg_manager *);
void				htxd_delete_ecg_manager(void);
extern int			htxd_get_dr_sem_key(void);
extern void			htxd_set_dr_sem_id(int new_dr_sem_id);
extern int			htxd_get_dr_sem_id(void);
extern int			htxd_get_dr_reconfig_restart(void);
extern void			htxd_set_dr_reconfig_restart(int value);
extern void			htxd_set_htx_path(char *);
extern char *		htxd_get_htx_path(void);
extern int			htxd_get_msg_queue_id(void);
extern void			htxd_set_program_name(char *);
extern char *		htxd_get_program_name(void);
extern int			htxd_get_number_of_running_ecg(void);  /* not using now */
extern void			htxd_set_equaliser_pid(int);
extern pid_t		htxd_get_equaliser_pid(void);
extern void			htxd_set_equaliser_shm_addr(union shm_pointers);
extern union shm_pointers	htxd_get_equaliser_shm_addr(void);
extern void			htxd_set_equaliser_semhe_id(int);
extern int			htxd_get_equaliser_semhe_id(void);
extern void			htxd_set_equaliser_conf_file(char *);
extern char *		htxd_get_equaliser_conf_file(void);
extern void         htxd_set_equaliser_sys_cpu_util(char *);
extern char *       htxd_get_equaliser_sys_cpu_util(void);

extern void			htxd_set_equaliser_debug_flag(int);
extern int			htxd_get_equaliser_debug_flag(void);
extern void			htxd_set_equaliser_offline_cpu_test_flag(int);
extern int			htxd_get_equaliser_offline_cpu_test_flag(void);
extern int          htxd_get_equaliser_time_quantum(void);
extern void         htxd_set_equaliser_time_quantum(int value);
extern int          htxd_get_equaliser_startup_time_delay(void);
extern void         htxd_set_equaliser_startup_time_delay(int value);
extern int          htxd_get_equaliser_log_duration(void);
extern void         htxd_set_equaliser_log_duration(int value);
extern int          htxd_get_equaliser_pattern_length(void);
extern void         htxd_set_equaliser_pattern_length(int value);
extern int			htxd_is_hang_monitor_initialized(void);
extern void			htxd_remove_hang_monitor(void);
extern int			htxd_is_hotplug_monitor_initialized(void);
extern int 			htxd_is_time_driven_run_monitor_initialized(void);
extern void			htxd_remove_time_driven_run_monitor(void);
extern void			htxd_remove_hotplug_monitor(void);
extern int			htxd_get_system_header_info_error_count(void);
extern void			htxd_set_system_header_info_shm_id(int);
extern int			htxd_get_system_header_info_shm_id(void);
extern void			htxd_set_exer_table_shm_id(int);
extern int			htxd_get_exer_table_shm_id(void);
extern tsys_hdr *	htxd_get_system_header_info(void);
extern void			htxd_set_init_syscfg_flag(int);
extern int			htxd_is_init_syscfg(void);
extern int			htxd_get_test_running_state(void);
extern void			htxd_set_test_running_state(int);
extern int			htxd_get_ecg_sem_id(void);
extern int			htxd_get_equaliser_offline_cpu_flag(void);
extern void			htxd_set_equaliser_offline_cpu_flag(int value);
extern int			htxd_stop_stop_watch_monitor(htxd_thread *p_stop_watch_monitor_thread);
extern int			htxd_start_stop_watch_monitor(htxd_thread *p_stop_watch_monitor_thread);
extern void			htxd_remove_stop_watch_monitor(void);
extern int			htxd_is_stop_watch_monitor_initialized(void);

#endif
