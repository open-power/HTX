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
/* @(#)37	1.1  src/htx/usr/lpp/htx/bin/htxd/htxd_common_define.h, htxd, htxubuntu 7/17/13 08:39:18 */



/*
 * Note: this header contains the common macros for both daemon and command program 
 */

#ifndef HTXD__COMMON__DEFINE__HEADER
#define HTXD__COMMON__DEFINE__HEADER 

#define	HTXD_DEFAULT_PORT		3492

#define MAX_ECG_NAME_LENGTH		512
#define MAX_OPTION_LIST_LENGTH	512

#define MAX_DEVICE_PER_SCREEN 34
#define DEV_ID_MAX_LENGTH 40

typedef struct
{
	char device_name[DEV_ID_MAX_LENGTH];
	char run_status[5];
	char day_of_year[20];
	char last_update_time[11];
	char cycle_count[28];
	char stanza_count[28];
	char last_error_day[11];
	char last_error_time[11];
	char slot_port[12];
	char error_count[28];
	
} query_device_entry;

typedef struct
{
	char running_mdt_name[MAX_ECG_NAME_LENGTH];
	char current_system_time[26];
	char test_started_time[26];
	char device_entry_count[26];
	char display_device_count[26];
	char system_error_flag[5];
} query_header;

typedef union
{
	query_header *p_query_header;
	query_device_entry *p_query_entry;
} query_details;


typedef struct
{
	char		device_name[DEV_ID_MAX_LENGTH];
	char		device_status[16];
	unsigned short	slot;
	unsigned short	port;
	char		adapt_desc[12];
	char		device_desc[16];
} activate_halt_entry;



#endif

