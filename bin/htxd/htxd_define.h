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
/* @(#)39	1.4  src/htx/usr/lpp/htx/bin/htxd/htxd_define.h, htxd, htxubuntu 9/15/15 20:28:07 */



#ifndef HTXD__LIMITS__HEADER
#define HTXD__LIMITS__HEADER



#define MAX_DEVICE_INDEX_LENGTH 200
#define	HTX_ERR_MESSAGE_LENGTH 512

#define DEFAULT_ECG_NAME "mdt.bu"
#define RUN_SETUP_SCRIPT "exer_setup"
#define RUN_SETUP_ERROR_OUTPUT "res_setup"
#define HE_SCRIPT_FILE "hxsscripts"
#define HTX_STATS_FILE "htxstats"
#define HTX_STATS_SEND_FILE "htxstats.cmd_ln"
#define HTX_ERR_LOG_FILE "htxerr"
#define HTX_MSG_LOG_FILE "htxmsg"
#define HTX_CMD_RESULT_FILE "command_result"
#define HTX_VPD_SCRIPT "gen_vpd"
#define HTX_VPD_FILE "htx_vpd"
#define MDT_DIR "mdt"
#define MDT_LIST_FILE "mdt_list"
#define DR_MDT_NAME "mdt.dr"
#define HTXD_AUTOSTART_FILE ".htxd_autostart"
#define HTXD_BOOTME_SCRIPT "htxd_bootme"
#define HTX_SCREENS_DIR "screens"
#define RUNNING_MDT_STRING "running_mdt_string"
#define STATS_FILE_STRING "stats_file_string"
#define	MDT_LIST "mdt_list"
#define	BUILD_NET_LOG	"htxd_build_net_bpt_log"
#define	MDT_NET_LOG "htxd_mdt_net_log"
#define NO_BPT_NAME_PROVIDED "NO_BPT_NAME_PROVIDED"
#define CAMSS_MDT_UPDATE_SCRIPT "update_camss_mdt"
#define HTXD_TRACE_LOG	"htxd_trace"
#define HTXD_PROCESS_CHECK_LOG "htxd_process_check_log"
#define	HTXD_CREATE_MDT_LOG "htxd_create_mdt_log"
#define HTXD_LOG_FILE	"htxd_log"
#define HTXD_START_STOP_LOG	"htxd.start.stop.time"

#define HTXD_BOOTME_ON		1
#define HTXD_BOOTME_OFF		2
#define HTXD_BOOTME_STATUS	3
#define HTXD_SOFT_BOOTME	10
#define HTXD_SOFT_FAST_BOOTME	15
#define HTXD_HARD_BOOTME	20
#define HTXD_HARD_FAST_BOOTME	25
#define	HTXD_BOOTME_REBOOT "REBOOT_NO_MDT"
#define	HTXD_BOOTME_SCRIPT_ERROR_LOG	"htxd_bootme_scripts_error_log"
#ifdef __HTX_LINUX__
#define HTXD_BOOTME_USAGE_TEXT	"usage:\nhtxcmdline -bootme <operation> [mode:<mode type>] [period:<1,2,3, or 4>]\noperation:<on, off ,status or help>\nmode type:\n\tsoft  - reboot\n\tsoftf - reboot -f\n\thard  - shoudown\n\thardf - shutdown -f\nperiod:\n\t1 - every 20 minutes\n\t2 - every 30 minutes\n\t3 - every hour\n\t4 - every midnight\n\nexample: htxcmdline -bootme on mode:softf period:1\nNote: default values are from bootme rule file"
#else
#define HTXD_BOOTME_USAGE_TEXT	"usage:\nhtxcmdline -bootme <operation> [mode:<mode type>] [period:<1,2,3, or 4>]\noperation:<on, off ,status or help>\nmode type:\n\tsoft  - shutdown -r\n\tsoftf - shutdown -Fr\n\thard  - shutdown\n\thardf - shutdown -F\nperiod:\n\t1 - every 20 minutes\n\t2 - every 30 minutes\n\t3 - every hour\n\t4 - every midnight\n\nexample: htxcmdline -bootme on mode:softf period:1\nNote: default values are from bootme rule file"
#endif



#define EXTRA_DEVICE_ENTRIES 0

#define HTXD_PATH_MAX 1024

#define KYE_OFF_SET_LIMIT	1000
#define EXER_TABLE_LENGTH_LIMIT	1600

#define	EXER_RUNNING_CHECK_COUNT 200
#define WAIT_TIME_TO_STOP_EXER	4

#define HTX_COE 1
#define HTX_SOE 0

#define HTX_SINGLE_DEVICE	1
#define HTX_SCREEN_DEVICES	2
#define HTX_ALL_DEVICES		3

#define	COE_SOE_STATE		1
#define	ACTIVE_SUSPEND_STATE	2
#define	RUNNING_STATE		3

#define STRING_EXTRA_SPACE 5

#define SIGALRM_EXP_TIME 900

#define MSG_TOO_LONG 0x0001
#define BAD_GETTIMER 0x0002
#define BAD_MSGSND 0x0004
#define NO_MSG_QUEUE 0x0008

#define EXTRA_BUFFER_LENGTH 1024
#define	LINE_ENTRY_LENGTH 200

#ifndef GOOD
# define GOOD 0
#endif

#ifdef __HTX_LINUX__
# define SIGMAX SIGRTMAX
#endif

#define DUP_DEVICE_NOT_FOUND	0
#define DUP_DEVICE_FOUND	1
#define DUP_DEVICE_TERMINATED	2

#define HTXD_MDT_SHUTDOWN_MSG	190

#define HTXD_HTX_PROCESS_FOUND 51


/* daemon states */
#define		HTXD_DAEMON_STATE_IDLE			1
#define		HTXD_DAEMON_STATE_CREATING_MDT		2
#define		HTXD_DAEMON_STATE_SELECTING_MDT		4
#define		HTXD_DAEMON_STATE_SELECTED_MDT		8
#define		HTXD_DAEMON_STATE_STARTING_MDT		16
#define		HTXD_DAEMON_STATE_RUNNING_MDT		32
#define		HTXD_DAEMON_SHUTTING_DOWN_MDT		64
#define		HTXD_DAEMON_STATE_AUTOSTART_SETUP	128

/* command supported state */
#define		HTXD_COMMND_STATE_IDLE			HTXD_DAEMON_STATE_IDLE 
#define		HTXD_COMMND_STATE_RUNNING		HTXD_DAEMON_STATE_RUNNING_MDT
#define         HTXD_COMMND_STATE_SELECTED_RUNNING	(HTXD_DAEMON_STATE_SELECTED_MDT | HTXD_DAEMON_STATE_RUNNING_MDT)
#define         HTXD_COMMND_STATE_IDLE_SELECTED_RUNNING (HTXD_DAEMON_STATE_IDLE | HTXD_DAEMON_STATE_SELECTED_MDT | HTXD_DAEMON_STATE_RUNNING_MDT)
#define         HTXD_COMMND_STATE_IDLE_SELECTED		(HTXD_DAEMON_STATE_IDLE | HTXD_DAEMON_STATE_SELECTED_MDT)
#define         HTXD_COMMND_STATE_ALL			(HTXD_DAEMON_STATE_IDLE | HTXD_DAEMON_STATE_CREATING_MDT | HTXD_DAEMON_STATE_SELECTING_MDT | HTXD_DAEMON_STATE_SELECTED_MDT | HTXD_DAEMON_STATE_STARTING_MDT | HTXD_DAEMON_STATE_RUNNING_MDT | HTXD_DAEMON_SHUTTING_DOWN_MDT | HTXD_DAEMON_STATE_AUTOSTART_SETUP)



/* test states */
#define		HTXD_TEST_HALTED		0
#define		HTXD_TEST_ACTIVATED		1

#define		SCREEN_2_4_DEVICES_PER_PAGE	15
#define		SCREEN_5_ROWS 17
#define		SCREEN_5_SECTIONS 2
#define		SCREEN_5_ENTRIES_PER_PAGE ( (SCREEN_5_SECTIONS) * (SCREEN_5_ROWS))


extern char global_htx_home_dir[256];
extern char global_htx_log_dir[256];
extern char global_htxd_log_dir[256];
#endif
