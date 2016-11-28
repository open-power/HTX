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
/* @(#)55	1.3  src/htx/usr/lpp/htx/bin/htxd/htxd_option_method_list_mdt.c, htxd, htxubuntu 7/8/15 00:08:59 */



#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <dirent.h>
#include <string.h>
#include <errno.h>

#include "htxd.h"
#include "htxd_define.h"

#define DIR_ENTRY_LENGTH 256
#define LIST_ITEM_LENGTH 256
#define	BUFFER_LENGTH 1024

int htxd_list_files(char *path_name, char **file_list)
{
	DIR *p_dir;
	struct dirent * p_dir_entry;
	int i = 1;
	char list_item[LIST_ITEM_LENGTH];
	FILE *fp_command;
	char command_string[128];
	int dir_entry_count;



	sprintf(command_string, "ls -l %s | wc -l", path_name);	
	fp_command = popen(command_string, "r");
	fscanf(fp_command, "%d", &dir_entry_count);
	pclose(fp_command);

	*file_list = malloc((dir_entry_count * DIR_ENTRY_LENGTH) + BUFFER_LENGTH);
	if(*file_list == NULL) {
		return -1;
	}

	memset(*file_list, 0, (dir_entry_count * DIR_ENTRY_LENGTH) + BUFFER_LENGTH);

	p_dir = opendir(path_name);
		if(p_dir == NULL) {
		perror("dir open failed");
		return -1;
	}

	while ( (p_dir_entry = readdir(p_dir) ) != NULL) {
		if( p_dir_entry->d_name[0] == '.') { 	/* skip hidden filies			*/
			continue;
		}

		if((i % 2 == 0) && strlen(p_dir_entry->d_name) > 25) { 
			strcat(*file_list, "\n");
		}

		sprintf(list_item, "%2d) %-30s", i, p_dir_entry->d_name);
		strcat(*file_list, list_item);

		if((i % 2 == 1) && strlen(p_dir_entry->d_name) > 25) { 
			strcat(*file_list, "\n");
		}

		if( i % 2 == 0) {
			strcat(*file_list, "\n");
		}

		i++;
	}

	if(closedir(p_dir)) {
		perror("dir close failed");
	}


    return 0;
}


int htxd_option_method_list_mdt(char **result)
{
	int return_code = 0;
	char temp_string[300];

	sprintf(temp_string, "%s/%s", global_htx_home_dir, MDT_DIR);
	return_code = htxd_list_files(temp_string, result);

	return return_code;
}

