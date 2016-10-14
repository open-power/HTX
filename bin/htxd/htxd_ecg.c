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
/* @(#)42	1.5  src/htx/usr/lpp/htx/bin/htxd/htxd_ecg.c, htxd, htxubuntu 8/23/15 23:34:18 */



#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#define PARMS(x) x
#include <cfgclibdef.h>

#include "htxd_ecg.h"
#include "htxd_ipc.h"
#include "htxd_profile.h"
#include "htxd_util.h"
#include "htxd_define.h"
#include "htxd_instance.h"
#include "htxd_trace.h"



int htxd_set_device_restart_status(htxd_ecg_info *, int);

/* a generic function to process a command for all active ecg */
int htxd_process_all_active_ecg_device(ecg_device_process_function process_action, char *device_list, char *command_result)
{
	htxd *htxd_instance;
	htxd_ecg_info * p_ecg_info_list;
	int return_code = 0;


	htxd_instance = htxd_get_instance();

	p_ecg_info_list = htxd_get_ecg_info_list(htxd_instance->p_ecg_manager);

	while(p_ecg_info_list != NULL) {
		return_code = process_action(p_ecg_info_list, device_list, command_result);
		p_ecg_info_list = p_ecg_info_list->ecg_info_next;
	}

	return return_code;
}



/* a generic function to process a command for all devices in an  ecg */
int htxd_process_all_active_ecg(ecg_process_function process_action, char *command_result)
{
	htxd *htxd_instance;
	htxd_ecg_info * p_ecg_info_list;
	int return_code = 0;


	htxd_instance = htxd_get_instance();

	p_ecg_info_list = htxd_get_ecg_info_list(htxd_instance->p_ecg_manager);

	while(p_ecg_info_list != NULL) {
		return_code = process_action(p_ecg_info_list, command_result);
		p_ecg_info_list = p_ecg_info_list->ecg_info_next;
	}

	return return_code;
}



/* get number of devices present in an ecg */
int htxd_get_number_of_device(char *ecg_name, char *error_msg)
{
	int number_of_devices = 0;
	char stanza[4096];
	int return_code;
	CFG__SFT *ecg_fd;
	char error_tag[40];

	error_msg[0] = '\0';

	ecg_fd = cfgcopsf(ecg_name);
	if(ecg_fd == (CFG__SFT *) NULL) {
		sprintf(error_msg, "Unable to open ECG file.%d (%s)", errno, ecg_name);
		return -1;
	}

	while( (return_code = cfgcrdsz (ecg_fd, stanza, sizeof (stanza), (char *) NULL) ) == CFG_SUCC) {
		number_of_devices++;
	}

	if(return_code != CFG_EOF) {
		number_of_devices = -1;
		switch (return_code) {
			case CFG_SZNF:
				sprintf(error_tag, "SZNF");
				break;

			case CFG_SZBF:
				sprintf(error_tag, "SZBF");
				break;

			case CFG_UNIO:
				sprintf(error_tag, "UNIO");
				break;

			default:
				sprintf(error_tag, "????");
				break;
		}
		strcat(error_msg, error_tag);
		sprintf(error_tag, ", return code = %d", return_code);
		strcat(error_msg, error_tag);
		return -1;
	}

	if (cfgcclsf (ecg_fd) != CFG_SUCC) {
		sprintf(error_msg, "Unable to close ECG file.%d (%s)", errno, ecg_name);
		return -1;
	}
	return number_of_devices;
}



/* returns number of ecgs currently running */
int htxd_get_ecg_list_length(htxd_ecg_manager* ecg_manager)
{
	if(ecg_manager == 0) {
		return 0;
	}
	return ecg_manager->ecg_list_length;
}



/* returns the ecg info list from ecg manager */
htxd_ecg_info * htxd_get_ecg_info_list(htxd_ecg_manager* ecg_manager)
{
	return ecg_manager->ecg_info_list;
}



/* returns the list of active ecg names */
void htxd_get_active_ecg_name_list(htxd_ecg_manager* ecg_manager, char *ecg_list)
{
	htxd_ecg_info * p_ecg_info_list;

	ecg_list[0] = '\0';
	p_ecg_info_list = ecg_manager->ecg_info_list;
	while(p_ecg_info_list != NULL) {
		strcat(ecg_list, p_ecg_info_list->ecg_name);
		strcat(ecg_list, "\n");
		p_ecg_info_list = p_ecg_info_list->ecg_info_next;
	}

}



/* initailize ecg manager object */
void htxd_init_ecg_manager(htxd_ecg_manager* ecg_manager)
{
	ecg_manager->ecg_info_list = NULL;
	ecg_manager->loaded_device_count = 0;
	ecg_manager->ecg_device_count = 0;
	ecg_manager->ecg_list_length = 0;
	ecg_manager->current_loading_ecg_info = NULL;
	ecg_manager->message_queue_key = 0;
	ecg_manager->message_queue_id = -1;
	ecg_manager->ecg_list_shm_key = SHMKEY;
	ecg_manager->ecg_list_shm_id = 0;
	ecg_manager->exer_table = (texer_list *)0;
	ecg_manager->exer_table_length = 0;
	ecg_manager->system_header_info = (tsys_hdr *)0;
	ecg_manager->selected_ecg_name[0] = '\0';
	ecg_manager->running_ecg_name[0] = '\0';
}



/* allocate ecg manager */
htxd_ecg_manager * create_ecg_manager(void)
{
	htxd_ecg_manager *p_ecg_manager = NULL;
	int exer_table_length_limit;

	p_ecg_manager = malloc(sizeof(htxd_ecg_manager) );
	if(p_ecg_manager == NULL) {
		exit(1);
	}
	memset(p_ecg_manager, 0, sizeof(htxd_ecg_manager) );

	htxd_init_ecg_manager(p_ecg_manager);
	htxd_set_ecg_manager(p_ecg_manager);

	/* allocating message queue */
	p_ecg_manager->message_queue_key = MSGLOGKEY;
	if(p_ecg_manager->message_queue_id == -1) {
		p_ecg_manager->message_queue_id = htxd_create_message_queue(p_ecg_manager->message_queue_key);
	}

	/* allocating exer table */
	if(p_ecg_manager->exer_table == 0) {
		exer_table_length_limit = htxd_get_profile_max_exer_entries();
		/* p_ecg_manager->exer_table = (texer_list *)htxd_create_exer_table(EXER_TABLE_LENGTH_LIMIT); */
		p_ecg_manager->exer_table = (texer_list *)htxd_create_exer_table(exer_table_length_limit);
	}

	/* allocating miscellaneous shared memory */
	if(p_ecg_manager->system_header_info == 0) {
		p_ecg_manager->system_header_info = (tsys_hdr *)htxd_create_system_header_info();
	}

	htxd_set_system_header_info_shm_with_max_exer_entries(p_ecg_manager, htxd_get_max_exer_entries() );

	return p_ecg_manager;
}



/* walk through ecg info list and display info  for DEBUG purpose*/
void htxd_display_ecg_info_list(void)
{
	htxd_ecg_manager *p_temp_ecg_manager;
 	htxd_ecg_info * p_ecg_info_list;

	p_temp_ecg_manager = htxd_get_ecg_manager();

	printf("[DEBUG] : <==================== START: ECG INFO DETAILS =========================\n");

        p_ecg_info_list = p_temp_ecg_manager->ecg_info_list;
        while(p_ecg_info_list != NULL) {
		printf("[DEBUG] : <<<===================================================================\n");
		printf("DEBUG: htxd_display_ecg_info_list() : ecg_name = <%s>\n", p_ecg_info_list->ecg_name);
		printf("DEBUG: htxd_display_ecg_info_list() : ecg_shm_key = <%d>\n", p_ecg_info_list->ecg_shm_key);
                p_ecg_info_list = p_ecg_info_list->ecg_info_next;
		printf("[DEBUG] : ===================================================================>>>\n");
        }
	printf("[DEBUG] : ==================== END: ECGG INFO DETAILS =========================>>>\n");

}



/* allocate an ECG info node and attach to the ecg info list */
void htxd_allocate_ecg_info(htxd_ecg_manager* this_ecg_manager)
{
	htxd_ecg_info * p_temp_ecg_info;

	if(this_ecg_manager->ecg_info_list == NULL) {

		p_temp_ecg_info = malloc(sizeof(htxd_ecg_info) );
		if(p_temp_ecg_info == NULL) {
			exit(1);
		}
		this_ecg_manager->current_loading_ecg_info = this_ecg_manager->ecg_info_list = p_temp_ecg_info;
		p_temp_ecg_info->ecg_info_next = NULL;
		p_temp_ecg_info->ecg_info_previous = NULL;

	} else {
		p_temp_ecg_info = this_ecg_manager->ecg_info_list;

		while(p_temp_ecg_info->ecg_info_next != NULL) {
			p_temp_ecg_info = p_temp_ecg_info->ecg_info_next;
		}
		p_temp_ecg_info->ecg_info_next = malloc(sizeof(htxd_ecg_info) );
		if(p_temp_ecg_info->ecg_info_next == NULL) {
			exit(1);
		}
		p_temp_ecg_info->ecg_info_next->ecg_info_previous = p_temp_ecg_info;
		p_temp_ecg_info = p_temp_ecg_info->ecg_info_next;
		p_temp_ecg_info->ecg_info_next = NULL;
		this_ecg_manager->current_loading_ecg_info = p_temp_ecg_info;
	}
}



/*void htxd_remove_ecg_info_node(htxd_ecg_info* p_temp_ecg_info)
{
	if(p_temp_ecg_info != NULL) {
		if(p_temp_ecg_info->ecg_info_previous != NULL) {
			p_temp_ecg_info->ecg_info_previous->ecg_info_next = p_temp_ecg_info->ecg_info_next;
		}
		if(p_temp_ecg_info->ecg_info_next != NULL) {
			p_temp_ecg_info->ecg_info_next->ecg_info_previous = p_temp_ecg_info->ecg_info_previous;
		}
	}
}



void htxd_remove_ecg_info_list(htxd_ecg_manager *this_ecg_manager)
{
	this_ecg_manager->ecg_info_list = NULL;
}
*/



void htxd_remove_ecg_info_node(htxd_ecg_manager *this_ecg_manager, htxd_ecg_info* p_temp_ecg_info)
{
	int current_ecg_info_list_length;

	current_ecg_info_list_length = htxd_get_ecg_list_length(this_ecg_manager);
	if(current_ecg_info_list_length == 1) {
		this_ecg_manager->ecg_info_list = NULL;
	} else {
		if(p_temp_ecg_info->ecg_info_previous != NULL) {
			p_temp_ecg_info->ecg_info_previous->ecg_info_next = p_temp_ecg_info->ecg_info_next;
		} else {
			this_ecg_manager->ecg_info_list = p_temp_ecg_info->ecg_info_next;
			p_temp_ecg_info->ecg_info_next->ecg_info_previous = p_temp_ecg_info->ecg_info_previous;
		}
		if(p_temp_ecg_info->ecg_info_next != NULL) {
			p_temp_ecg_info->ecg_info_next->ecg_info_previous = p_temp_ecg_info->ecg_info_previous;
		}
	}
	(this_ecg_manager->ecg_list_length)--;
}



/* set ECH header values in shared memory  */
void htxd_set_shm_header_values(htxd_ecg_manager *this_ecg_manager)
{
	htxd_ecg_info *p_current_ecg_info = this_ecg_manager->current_loading_ecg_info;


	p_current_ecg_info->ecg_shm_addr.hdr_addr->num_entries = p_current_ecg_info->ecg_exerciser_entries;
	p_current_ecg_info->ecg_shm_addr.hdr_addr->max_entries = p_current_ecg_info->ecg_max_exerciser_entries;
	p_current_ecg_info->ecg_shm_addr.hdr_addr->pseudo_entry_0 = p_current_ecg_info->ecg_max_exerciser_entries + htxd_get_max_add_device();
}




/* returns dupplicate device status */
int htxd_get_device_dup_status(htxd_ecg_manager *p_ecg_manager, char *device_name)
{
	int exer_position_in_exer_table = -1;

	exer_position_in_exer_table = htxd_get_exer_position_in_exer_table_by_exer_name(p_ecg_manager->exer_table, device_name);
	if(exer_position_in_exer_table == -1) {
		return DUP_DEVICE_NOT_FOUND;
	} else {
		if(p_ecg_manager->exer_table[exer_position_in_exer_table].exer_pid == 0) {
			return DUP_DEVICE_TERMINATED;
		}
		return DUP_DEVICE_FOUND;
	}
}


/* read ECG stanza and set values shared memory for each HE */
int htxd_extract_ecg_to_shm_HE(htxd_ecg_manager *this_ecg_manager, char *stanza, int stanza_length,union shm_pointers shm_point, char **default_stanza,  CFG__SFT * mdt_fd, int *p_dup_flag, char *p_device_name)
{
    char workstr[128], str_tmp[16];      /* work string                  */
    int rc;                      /* general return code          */
    char *shm_work_ptr;          /* char pointer into shm        */
    char *stanza_ptr;            /* pointer into stanza          */
    int i;
    int dup_device_status;



    if (default_stanza == NULL || *default_stanza == NULL)
      /* read the next stanza */
    rc = cfgcrdsz (mdt_fd, stanza, stanza_length, (char *) NULL);
    else {
    rc = cfgcrdsz (mdt_fd, stanza, stanza_length, "default");
    switch (rc) {
        case CFG_SUCC:           /* default found, read next stanza */
        rc = cfgcrdsz (mdt_fd, stanza, stanza_length, (char *) NULL);
        break;
        case CFG_SZNF:           /* not found, rewind, set it, and get next */
        cfgcrwnd (mdt_fd);
        (void) cfgcsvdflt (mdt_fd, *default_stanza);
        rc = cfgcrdsz (mdt_fd, stanza, stanza_length, (char *) NULL);
        break;
    //    default:                 /* handle problem later */
    }                         /* switch */
    *default_stanza = NULL;   /* don't want to see it again */
    }                            /* if */

   /* get next stanza */
    if (rc != CFG_SUCC) {
    return (rc);              /* no next stanza or some other problem */
    }

    /**********************************************************/
   /* Now setup the HE entry from the stanza that is in hand */
    /**********************************************************/

    /* set device id */
    shm_work_ptr = &(shm_point.HE_addr->sdev_id[0]);
    stanza_ptr = &(stanza[0]);
    do {
    *shm_work_ptr = *stanza_ptr;
    shm_work_ptr++;
    stanza_ptr++;
    }
    while (*stanza_ptr != ':');  /* enddo */
    *shm_work_ptr = '\0';        /* terminating 0 for text string    */
	strcpy(p_device_name, shm_point.HE_addr->sdev_id);

	*p_dup_flag = FALSE;
	dup_device_status = htxd_get_device_dup_status(this_ecg_manager, shm_point.HE_addr->sdev_id);
	if(dup_device_status == DUP_DEVICE_FOUND) {
		*p_dup_flag = TRUE;

		return rc;
	}

	if(dup_device_status == DUP_DEVICE_TERMINATED) {
		*p_dup_flag = TRUE;
	}

   /* set Hardware Exerciser Name */
    cfgcskwd ("HE_name", stanza, workstr);
    (void) strcpy (shm_point.HE_addr->HE_name, htxd_unquote (workstr));

   /* set Adapter Description */
    cfgcskwd ("adapt_desc", stanza, workstr);
    (void) strcpy (shm_point.HE_addr->adapt_desc, htxd_unquote (workstr));

   /* set Device Description */
    cfgcskwd ("device_desc", stanza, workstr);
    (void) strcpy (shm_point.HE_addr->device_desc, htxd_unquote (workstr));

   /* set EMC Rules File Name */
    cfgcskwd ("emc_rules", stanza, workstr);
    (void) strcpy (shm_point.HE_addr->emc_rules, htxd_unquote (workstr));

   /* set REG Rules File Name */
    cfgcskwd ("reg_rules", stanza, workstr);
    (void) strcpy (shm_point.HE_addr->reg_rules, htxd_unquote (workstr));

    cfgcskwd ("log_vpd", stanza, workstr);
    (void) strcpy (str_tmp, htxd_unquote (workstr));
    switch (str_tmp[0]) {

    case 'y':
    case 'Y':
        shm_point.HE_addr->log_vpd = 1;
        //print_log(LOGMSG,"%s:Detailed Error log is set\n", shm_point.HE_addr->HE_name);
        break;
    case 'n':
    case 'N':
    default:
        shm_point.HE_addr->log_vpd = 0;
        break;
	}

    cfgcskwd ("dup_device", stanza, workstr);
    (void) strcpy (str_tmp, htxd_unquote (workstr));
    switch (str_tmp[0]) {

    case 'y':
    case 'Y':
        shm_point.HE_addr->dup_device = 1;
        //print_log(LOGMSG,"%s:dup device is set\n", shm_point.HE_addr->HE_name);
        break;
    case 'n':
    case 'N':
    default:
        shm_point.HE_addr->dup_device = 0;
        break;
    }


   /* set DMA Channel */
    cfgcskwd ("dma_chan", stanza, workstr);
    shm_point.HE_addr->dma_chan = (unsigned short) atoi (htxd_unquote (workstr));

   /* set hft number */
#ifdef HTX_REL_tu320
    cfgcskwd ("hft", stanza, workstr);
    workint = (unsigned short) atoi (htxd_unquote (workstr));
    if ((workint < 0) || (workint > 3))
    shm_point.HE_addr->hft_num = 0;
    else
    shm_point.HE_addr->hft_num = workint;
#endif

   /* set Idle Time */
    cfgcskwd ("idle_time", stanza, workstr);
    shm_point.HE_addr->idle_time = (unsigned short) atoi (htxd_unquote (workstr));

   /* set Interrupt Level */
    cfgcskwd ("intrpt_lev", stanza, workstr);
    shm_point.HE_addr->intrpt_lev = (unsigned short) atoi (htxd_unquote (workstr));

   /* set Load Sequence */
    cfgcskwd ("load_seq", stanza, workstr);
    shm_point.HE_addr->load_seq = (unsigned short) atoi (htxd_unquote (workstr));

   /* set Maximum Run Time */
    cfgcskwd ("max_run_tm", stanza, workstr);
    shm_point.HE_addr->max_run_tm = (unsigned short) atoi (htxd_unquote (workstr));

   /* set Port Number */
    cfgcskwd ("port", stanza, workstr);
    shm_point.HE_addr->port = (unsigned short) atoi (htxd_unquote (workstr));

   /* set Priority */
    cfgcskwd ("priority", stanza, workstr);
    shm_point.HE_addr->priority = (unsigned short) atoi (htxd_unquote (workstr));

   /* set Slot Number */
    cfgcskwd ("slot", stanza, workstr);
    shm_point.HE_addr->slot = (unsigned short) atoi (htxd_unquote (workstr));
    if (!strcmp (htxd_unquote (workstr), "0"))        /* modifications for 354660 */
    strcpy (workstr, "NA          ");
    else
    strcpy (workstr, htxd_unquote (workstr));
    for (i = strlen (workstr); i < 13; i++)
    workstr[i] = ' ';
    workstr[13] = 0;

    (void) strcpy (shm_point.HE_addr->slot_port, htxd_unquote (workstr));

   /* clear PID Number */
    shm_point.HE_addr->PID = 0;
    shm_point.HE_addr->is_child = 1;

   /* set Max cycles */
    cfgcskwd ("max_cycles", stanza, workstr);
    shm_point.HE_addr->max_cycles = (unsigned int) atoi (htxd_unquote (workstr));

   /* set Continue on Error Flag */
    cfgcskwd ("cont_on_err", stanza, workstr);
    htxd_unquote (workstr);
    if ((workstr[0] == 'y') || (workstr[0] == 'Y'))
    shm_point.HE_addr->cont_on_err = 1;
    else {
    shm_point.HE_addr->cont_on_err = 0;
    }                            /* endif */

   /* set Halt on Error Severity Code Level */
    cfgcskwd ("halt_level", stanza, workstr);
    htxd_unquote (workstr);

    if (strcmp (workstr, "0") == 0)
    shm_point.HE_addr->halt_sev_level = 0;
    else if (strcmp (workstr, "1") == 0)
    shm_point.HE_addr->halt_sev_level = 1;
    else if (strcmp (workstr, "2") == 0)
    shm_point.HE_addr->halt_sev_level = 2;
    else if (strcmp (workstr, "3") == 0)
    shm_point.HE_addr->halt_sev_level = 3;
    else if (strcmp (workstr, "4") == 0)
    shm_point.HE_addr->halt_sev_level = 4;
    else if (strcmp (workstr, "5") == 0)
    shm_point.HE_addr->halt_sev_level = 5;
    else if (strcmp (workstr, "6") == 0)
    shm_point.HE_addr->halt_sev_level = 6;
    else if (strcmp (workstr, "7") == 0)
    shm_point.HE_addr->halt_sev_level = 7;
    else if (strcmp (workstr, "HTX_HE_HARD_ERROR") == 0)
    shm_point.HE_addr->halt_sev_level = HTX_HE_HARD_ERROR;
    else if (strcmp (workstr, "HTX_HE_SOFT_ERROR") == 0)
    shm_point.HE_addr->halt_sev_level = HTX_HE_SOFT_ERROR;
    else
    shm_point.HE_addr->halt_sev_level = HTX_HE_HARD_ERROR;

   /* set error acknowledged flag */
    shm_point.HE_addr->err_ack = 1;

   /* set start_halted flag */
   /*
    * If HE set to halt on startup, set the appropriate semaphone AFTER
    * the shared memory has been sorted.
    */
    cfgcskwd ("start_halted", stanza, workstr);
    htxd_unquote (workstr);
    if ((workstr[0] == 'y') || (workstr[0] == 'Y'))
    shm_point.HE_addr->start_halted = 1;
    else
    shm_point.HE_addr->start_halted = 0;

   /* set virtual terminal number */
#ifdef HTX_REL_tu320
    shm_point.HE_addr->VT_num = 0;
#endif

   /* set Time of Last Error */
    shm_point.HE_addr->tm_last_err = 0;

   /* set Time of Last call to HTX_update() */
    shm_point.HE_addr->tm_last_upd = -1;

   /* set Number of Other Bad Operations */
    shm_point.HE_addr->bad_others = 0;

   /* set Number of Bad Reads */
    shm_point.HE_addr->bad_reads = 0;

   /* set Number of Bad Writes */
    shm_point.HE_addr->bad_writes = 0;

   /* set Number of Bytes Read */
    shm_point.HE_addr->bytes_read1 = 0;
    shm_point.HE_addr->bytes_read2 = 0;

   /* set Number of Bytes Written */
    shm_point.HE_addr->bytes_writ1 = 0;
    shm_point.HE_addr->bytes_writ2 = 0;

   /* set Number of Other Good Operations */
    shm_point.HE_addr->good_others = 0;

   /* set Number of Good Reads */
    shm_point.HE_addr->good_reads = 0;

   /* set Number of Good Writes */
    shm_point.HE_addr->good_writes = 0;

    return (rc);                 /* should always be CFG_SUCC here */
}



/* update exer table entry in shared memory for an exerciser */
int htxd_update_exer_table(htxd_ecg_manager *this_ecg_manager, union shm_pointers shm_point, int exer_position_in_ecg)
{
	htxd_ecg_info *p_current_ecg_info;
	texer_list * p_exer_table;
	int exer_position_in_exer_table;

	p_current_ecg_info = this_ecg_manager->current_loading_ecg_info;

	p_exer_table = this_ecg_manager->exer_table;
	exer_position_in_exer_table = htxd_get_exer_position_in_exer_table_by_exer_name(p_exer_table, shm_point.HE_addr->sdev_id);
	if(exer_position_in_exer_table == -1) {
		exer_position_in_exer_table = this_ecg_manager->exer_table_length;
		(this_ecg_manager->exer_table_length)++;
	}
	strcpy(p_exer_table[exer_position_in_exer_table].dev_name, shm_point.HE_addr->sdev_id);
	p_exer_table[exer_position_in_exer_table].ecg_shm_key = p_current_ecg_info->ecg_shm_key;
	p_exer_table[exer_position_in_exer_table].ecg_sem_key = p_current_ecg_info->ecg_sem_key;
	p_exer_table[exer_position_in_exer_table].ecg_semhe_id = p_current_ecg_info->ecg_sem_id;
	p_exer_table[exer_position_in_exer_table].exer_pos = exer_position_in_ecg;
	p_exer_table[exer_position_in_exer_table].exer_addr = shm_point;
	p_exer_table[exer_position_in_exer_table].ecg_exer_addr = shm_point;

	return 0;
}



/* returns the device position in shared memory (HE) */
int htxd_get_device_position_in_shm_HE(htxd_ecg_info *p_ecg_info, char *p_device_name)
{
	int i;
	struct htxshm_HE *p_HE;
	int device_position = -1;


	p_HE = (struct htxshm_HE *)(p_ecg_info->ecg_shm_addr.hdr_addr + 1);
	for(i = 0; i < p_ecg_info->ecg_shm_exerciser_entries ; i++) {
		if(strcmp(p_HE->sdev_id, p_device_name) == 0) {
			device_position = i;
			break;
		}
		p_HE++;
	}

	return device_position;
}



/* decides whether the specified device is eligible for DR restart */
int htxd_is_device_need_dr_restart(htxd_ecg_info *p_ecg_info, char *p_device_name)
{
	int i;
	struct htxshm_HE *p_HE;
	int restart_flag = FALSE;


	p_HE = (struct htxshm_HE *)(p_ecg_info->ecg_shm_addr.hdr_addr + 1);
	for(i = 0; i < p_ecg_info->ecg_shm_exerciser_entries ; i++) {
		if(strcmp(p_HE->sdev_id, p_device_name) == 0) {
			if( (p_HE->PID == 0) &&  (p_HE->DR_term == 1) ) {
				restart_flag = TRUE;
			}
			break;
		}
		p_HE++;
	}

	return restart_flag;
}



int htxd_set_shm_with_exercisers_values_for_dr_restart(char *dr_ecg_name)
{
	CFG__SFT *ecg_fd;
	int return_code = 0;
	htxd_ecg_info *p_running_ecg_info;
	char *running_ecg_name;
	htxd_ecg_manager *p_ecg_manager;
	char default_ecg_stanza[4096];
	char ecg_stanza[4096];
	union shm_pointers shm_point;
	int dup_flag = 0;
	char temp_device_name[80];
	int device_position;
	char trace_string[256];



	ecg_fd = cfgcopsf (dr_ecg_name);
	if(ecg_fd == (CFG__SFT *) NULL) {
		HTXD_TRACE(LOG_ON, "htxd_set_shm_exercisers_values_for_dr_restart() return: ecg_fd == (CFG__SFT *) NULL");
		return -1;
	}

	return_code = cfgcrdsz (ecg_fd, default_ecg_stanza, sizeof (default_ecg_stanza), "default");
	if(return_code != CFG_SUCC) {
		HTXD_TRACE(LOG_ON, "htxd_set_shm_exercisers_values_for_dr_restart() return: return_code != CFG_SUCC");
		return -1;
	}

	p_ecg_manager = htxd_get_ecg_manager();
	running_ecg_name = htxd_get_running_ecg_name();

	p_running_ecg_info = htxd_get_ecg_info_node(p_ecg_manager, running_ecg_name);

	shm_point.hdr_addr = p_running_ecg_info->ecg_shm_addr.hdr_addr + 1;
	shm_point.HE_addr += p_running_ecg_info->ecg_shm_exerciser_entries;
	while( (return_code =  htxd_extract_ecg_to_shm_HE(p_ecg_manager, ecg_stanza, sizeof (ecg_stanza), shm_point,(char **) NULL, ecg_fd, &dup_flag, temp_device_name)) == CFG_SUCC){
		if(dup_flag == FALSE) {
			htxd_update_exer_table(p_ecg_manager, shm_point, 8);
			(shm_point.HE_addr)++;
			(p_running_ecg_info->ecg_shm_exerciser_entries)++;
			p_running_ecg_info->ecg_shm_addr.hdr_addr->num_entries = p_running_ecg_info->ecg_shm_exerciser_entries;
			(p_running_ecg_info->ecg_shm_addr.hdr_addr->max_entries)++;
		} else {
			if(htxd_is_device_need_dr_restart(p_running_ecg_info, temp_device_name) == FALSE) {
				continue;
			}
		}

		device_position = htxd_get_device_position_in_shm_HE(p_running_ecg_info, temp_device_name);
		if(device_position == -1) {
			HTXD_TRACE(LOG_ON, "Inavlid device position in HE shared memory");
			continue;
		}
	/*	htxd_display_ecg_info_list();
		htxd_display_exer_table(); */
		htxd_set_device_restart_status(p_running_ecg_info, device_position);

	}
	if(return_code != CFG_EOF) {
		sprintf(trace_string, "htxd_extract_ecg_to_shm_HE returned with %d", return_code);
		HTXD_TRACE(LOG_ON, trace_string);
		return -1;
	}

	return_code = cfgcclsf (ecg_fd);
	if(return_code != CFG_SUCC) {
		sprintf(trace_string, "cfgcclsf returned with <%d>", return_code);
		HTXD_TRACE(LOG_ON, trace_string);
		return -1;
	}

	return 0;
}



/* update HE values and exer table for an exerciser shared memory */
int htxd_set_shm_exercisers_values(htxd_ecg_manager *this_ecg_manager, char *current_ecg_name)
{
	CFG__SFT *ecg_fd;
	int return_code;
	char default_ecg_stanza[4096];
	char ecg_stanza[4096];
	union shm_pointers shm_point;
	htxd_ecg_info *p_current_ecg_info;
	int exer_position_in_ecg = 0;
	int dup_flag = 0;
	char temp_device_name[80];


	ecg_fd = cfgcopsf (current_ecg_name);
	if(ecg_fd == (CFG__SFT *) NULL) {
		return -1;
	}

	return_code = cfgcrdsz (ecg_fd, default_ecg_stanza, sizeof (default_ecg_stanza), "default");
	if(return_code != CFG_SUCC) {
		return -1;
	}

	p_current_ecg_info = this_ecg_manager->current_loading_ecg_info;

	shm_point.hdr_addr = p_current_ecg_info->ecg_shm_addr.hdr_addr + 1;
	while( (return_code =  htxd_extract_ecg_to_shm_HE(this_ecg_manager, ecg_stanza, sizeof (ecg_stanza), shm_point,(char **) NULL, ecg_fd, &dup_flag, temp_device_name) ) == CFG_SUCC){
		if(dup_flag == FALSE) {
			htxd_update_exer_table(this_ecg_manager, shm_point, exer_position_in_ecg);
			exer_position_in_ecg++;
			(shm_point.HE_addr)++;
			(p_current_ecg_info->ecg_shm_exerciser_entries)++;
		}
	}
	if(return_code != CFG_EOF) {
		return -1;
	}

	if (cfgcclsf (ecg_fd) != CFG_SUCC) {
		return -1;
	}

	return 0;
}



/* calculate next possible offset value for shm and sem key values */
int htxd_get_next_key_offset(htxd_ecg_manager *this_ecg_manager)
{

	static int next_key_offset = 0;
	htxd_ecg_info * p_ecg_info_list;

	p_ecg_info_list = htxd_get_ecg_info_list(this_ecg_manager);
	if(p_ecg_info_list == NULL) {
		return next_key_offset;
	}

	while(next_key_offset < KYE_OFF_SET_LIMIT) {
		while(p_ecg_info_list != NULL) {
			if(p_ecg_info_list->ecg_shm_key == ECG_SHMKEY_START + next_key_offset) {
				next_key_offset++;
				break;
			}
			p_ecg_info_list = p_ecg_info_list->ecg_info_next;
		}
		if(p_ecg_info_list == NULL) {
			break;
		}
	}

	if(next_key_offset > KYE_OFF_SET_LIMIT) {
		next_key_offset = -1;
	}

	return next_key_offset;
}



int htxd_init_start_halted_exerciser_mode( htxd_ecg_info *p_ecg_info)
{
	struct htxshm_HE	*p_HE;
	int			i;
	int			return_code = 0;
	char			temp_string[256];


	p_HE = (struct htxshm_HE *)(p_ecg_info->ecg_shm_addr.hdr_addr + 1);
	for(i = 0; i < p_ecg_info->ecg_shm_exerciser_entries ; i++) {
		if(p_HE->start_halted == 1 ) {
			return_code = htxd_set_device_run_sem_status(p_ecg_info->ecg_sem_id, i, 1);
			if(return_code != 0) {
				sprintf(temp_string, "htxd_set_device_run_sem_status: returns error code <%d>\n", return_code);
				HTXD_TRACE(LOG_OFF, temp_string);
			}
		}
	}

	return 0;
}



/* get equaliser flag value from ECG */
int htxd_init_equaliser_info( htxd_ecg_info *p_ecg_info)
{
	int		return_code;
	CFG__SFT *	mdt_fd = NULL;
	char		default_mdt_snz[4096];
	char		temp_str[128];


	p_ecg_info->ecg_equaliser_info.enable_flag	= 0;
	p_ecg_info->ecg_equaliser_info.debug_flag	= 0;
	p_ecg_info->ecg_equaliser_info.config_file[0]	= '\0';
	p_ecg_info->ecg_equaliser_info.offline_cpu = 0;

	mdt_fd = cfgcopsf (p_ecg_info->ecg_name);
	if (mdt_fd == (CFG__SFT *) NULL) {
		sprintf (temp_str, "ERROR: Unable to open ECG file again.n");
		return -1;
	}

	return_code = cfgcrdsz (mdt_fd, default_mdt_snz, sizeof (default_mdt_snz), "default");
	if (return_code == CFG_SUCC) {
		return_code = cfgcskwd("equaliser_flag", default_mdt_snz, temp_str);
		if(return_code ==  CFG_SUCC) {
			p_ecg_info->ecg_equaliser_info.enable_flag = atoi(htxd_unquote(temp_str));
		}
		if(p_ecg_info->ecg_equaliser_info.enable_flag == 1) {
			putenv("EQUALISER_FLAG=1");
			return_code = cfgcskwd("cfg_file", default_mdt_snz, temp_str);
			if(return_code ==  CFG_SUCC) {
				strcpy( p_ecg_info->ecg_equaliser_info.config_file, htxd_unquote(temp_str));
				htxd_set_equaliser_conf_file(p_ecg_info->ecg_equaliser_info.config_file);
			}

			return_code = cfgcskwd("equaliser_debug_flag", default_mdt_snz, temp_str);
			if(return_code ==  CFG_SUCC) {
				p_ecg_info->ecg_equaliser_info.debug_flag = atoi(htxd_unquote(temp_str));
				htxd_set_equaliser_debug_flag(p_ecg_info->ecg_equaliser_info.debug_flag);
			}
			return_code = cfgcskwd("offline_cpu", default_mdt_snz, temp_str);
			if(return_code ==  CFG_SUCC) {
				p_ecg_info->ecg_equaliser_info.offline_cpu = atoi(htxd_unquote(temp_str));
				htxd_set_equaliser_offline_cpu_flag(p_ecg_info->ecg_equaliser_info.offline_cpu);
			#ifdef __HTX_LINUX__
				/* Also, get the SMT for core 0, if WOF test is enabled as this will be
				 * while binding the threads/processes to core 0.
				 */
				if (p_ecg_info->ecg_equaliser_info.offline_cpu == 1) {
					smt = get_smt_status(0);
					return_code = do_the_bind_proc(getpid());
					if (return_code < 0) {
						sprintf(temp_str, "Binding of htxd process to core 0 failed.\n");
						htxd_send_message(temp_str, 0, HTX_SYS_INFO, HTX_SYS_MSG);

					}
				}
			#endif
			}
			htxd_set_equaliser_shm_addr(p_ecg_info->ecg_shm_addr);
			htxd_set_equaliser_semhe_id(p_ecg_info->ecg_sem_id);
		} else {
		}

		if (cfgcclsf (mdt_fd) != CFG_SUCC) {
		}

	}

	return 0;
}



/* allocating IPC resources required for the new ECG */
int htxd_init_ecg_info(htxd_ecg_manager *this_ecg_manager, char *new_ecg_name)
{
	int number_of_devices;
	int max_number_of_devices;
	char error_msg[1024];
	htxd_ecg_info *p_current_ecg_info;
	int next_key_offset;
	char temp_string[256];


	HTXD_FUNCTION_TRACE(FUN_ENTRY, "htxd_init_ecg_info");

	sprintf(temp_string, "start processing ECG <%s>", new_ecg_name);
	HTXD_TRACE(LOG_OFF, temp_string);
	/* htxd_send_message (temp_string, 0, HTX_SYS_INFO, HTX_SYS_MSG); */

	htxd_allocate_ecg_info(this_ecg_manager);

	number_of_devices = htxd_get_number_of_device(new_ecg_name, error_msg);
	max_number_of_devices = number_of_devices + EXTRA_DEVICE_ENTRIES;

	p_current_ecg_info = this_ecg_manager->current_loading_ecg_info;
	p_current_ecg_info->ecg_info_next = NULL;

	p_current_ecg_info->ecg_exerciser_entries = number_of_devices;
	p_current_ecg_info->ecg_max_exerciser_entries = max_number_of_devices;
	p_current_ecg_info->ecg_shm_exerciser_entries = 0;
	strcpy( p_current_ecg_info->ecg_name, new_ecg_name);
	p_current_ecg_info->ecg_status = ECG_UNLOADED;
	sprintf(p_current_ecg_info->ecg_start_time, "%lu", time((long *) 0) );

	next_key_offset = htxd_get_next_key_offset(this_ecg_manager);

	p_current_ecg_info->ecg_shm_key = ECG_SHMKEY_START + next_key_offset;
	p_current_ecg_info->ecg_sem_key = ECG_SEMKEY_START + next_key_offset;


	HTXD_TRACE(LOG_OFF, "allocationg semaphore for ecg_info");
	/* allocating semaphore */
	if( (p_current_ecg_info->ecg_sem_id = htxd_init_sem(p_current_ecg_info->ecg_sem_key, max_number_of_devices) ) == -1 ) {
		return -1;
	}

	HTXD_TRACE(LOG_OFF, "allocationg shared memory for ecg_info");
	/* allocating shared memory */
	if( ( p_current_ecg_info->ecg_shm_addr.hdr_addr = htxd_init_shm(p_current_ecg_info->ecg_shm_key, max_number_of_devices,&(p_current_ecg_info->ecg_shm_id) ) ) == NULL ) {
		return -1;
	}

	/* setting shared memory with header values */
	HTXD_TRACE(LOG_OFF, "setting shm header values");
	htxd_set_shm_header_values(this_ecg_manager);

	/* setting shared memory with HE values */
	HTXD_TRACE(LOG_OFF, "setting shm HE values");
	htxd_set_shm_exercisers_values(this_ecg_manager, new_ecg_name);

	/* initialize equaliser info */
	HTXD_TRACE(LOG_OFF, "initialize equaliser_info");
	htxd_init_equaliser_info(p_current_ecg_info);

	/* initialize start-halted mode */
	htxd_init_start_halted_exerciser_mode(p_current_ecg_info);

	p_current_ecg_info->ecg_status = ECG_INACTIVE;

	p_current_ecg_info->ecg_shm_addr.hdr_addr->num_entries = p_current_ecg_info->ecg_shm_exerciser_entries;
	(this_ecg_manager->ecg_list_length)++;
	this_ecg_manager->ecg_device_count += p_current_ecg_info->ecg_shm_exerciser_entries;

	HTXD_FUNCTION_TRACE(FUN_EXIT, "htxd_init_ecg_info");
	return 0;
}



/* search exerciser in the exer table with pid */
int htxd_get_exer_position_in_exer_table_by_pid(texer_list *p_exer_table, pid_t search_pid)
{
	int exer_table_lenth;
	int i;
	int exer_position_in_exer_table = -1;

	exer_table_lenth = htxd_get_exer_table_length();

	for(i = 0; i < exer_table_lenth; i++) {
		if(search_pid == p_exer_table[i].exer_pid) {
			exer_position_in_exer_table  = i;
			break;
		}
	}

	return exer_position_in_exer_table;
}



/* search exerciser in the exer table with exer name */
int htxd_get_exer_position_in_exer_table_by_exer_name(texer_list *p_exer_table, char *search_exer_name)
{
	int exer_table_lenth;
	int i;
	int exer_position_in_exer_table = -1;

	exer_table_lenth = htxd_get_exer_table_length();

	for(i = 0; i < exer_table_lenth; i++) {
		if( strcmp(search_exer_name, p_exer_table[i].dev_name) == 0) {
			exer_position_in_exer_table  = i;
			break;
		}
	}

	return exer_position_in_exer_table;
}



/* update exer table with exer pid, usually after new exer is forked */
int htxd_update_exer_pid_in_exer_list(texer_list *p_exer_table, char *exer_name, pid_t new_exer_pid)
{

	int exer_position_in_exer_table;
	char trace_str[256];

	exer_position_in_exer_table = htxd_get_exer_position_in_exer_table_by_exer_name(p_exer_table, exer_name);
	if(exer_position_in_exer_table == -1) {
		sprintf(trace_str, "could not find exer_name <%s> in exer_table", exer_name);
		HTXD_TRACE(LOG_OFF, trace_str);
	} else {
		p_exer_table[exer_position_in_exer_table].exer_pid = new_exer_pid;
		/* printf("[DEBUG] : pid <%d> is updated for exer_name<%s> in exer_table at exer_position_in_exer_table <%d>\n", new_exer_pid, exer_name, exer_position_in_exer_table); fflush(stdout); */
	}

	return 0;
}



/* search in the ecg info list with ecg name for an ecg node in the list */
htxd_ecg_info * htxd_get_ecg_info_node(htxd_ecg_manager *this_ecg_manager, char *ecg_name)
{
	htxd_ecg_info *p_ecg_info_node = NULL;;

	p_ecg_info_node = htxd_get_ecg_info_list(this_ecg_manager);
	while(p_ecg_info_node != NULL) {
		if( strcmp(ecg_name, p_ecg_info_node->ecg_name) == 0) {
			break;
		}
		p_ecg_info_node = p_ecg_info_node->ecg_info_next;
	}

	return p_ecg_info_node;
}



void htxd_set_system_header_info_shm_with_current_shm_key(int curr_shm_key)
{
	htxd_ecg_manager *p_ecg_manager;

	p_ecg_manager = htxd_get_ecg_manager();

	p_ecg_manager->system_header_info->cur_shm_key = curr_shm_key;
}



void htxd_set_system_header_info_shm_with_max_exer_entries(htxd_ecg_manager *p_ecg_manager, int max_exer_entries)
{
	p_ecg_manager->system_header_info->max_exer_entries = max_exer_entries;
}



/* return currently running ecg count */
int htxd_get_running_ecg_count(void)
{
	int running_ecg_count = 0;


	running_ecg_count = htxd_get_ecg_list_length(htxd_get_ecg_manager());

	return running_ecg_count;
}


int htxd_get_running_ecg_list(char * ecg_name_list)
{

	htxd_ecg_manager *p_ecg_manager;
	htxd_ecg_info * p_ecg_info_list;
	int ecg_name_list_length;


	ecg_name_list[0] = '\0';
	p_ecg_manager = htxd_get_ecg_manager();

	p_ecg_info_list = htxd_get_ecg_info_list(p_ecg_manager);

	while(p_ecg_info_list != NULL) {
		strcat(ecg_name_list, p_ecg_info_list->ecg_name);
		strcat(ecg_name_list, ", ");
		p_ecg_info_list = p_ecg_info_list->ecg_info_next;
	}

	ecg_name_list_length = strlen(ecg_name_list);
	if(ecg_name_list_length > 0) {
		ecg_name_list[ecg_name_list_length - 2] = '\0';
	}

	return 0;
}



/* return currently running ecg name, valid when only 1 ecg is running */
char * htxd_get_running_ecg_name(void)
{
	htxd_ecg_manager *p_ecg_manager;

	p_ecg_manager = htxd_get_ecg_manager();

	return p_ecg_manager->ecg_info_list->ecg_name;
}



char * htxd_get_ecg_start_time(void)
{
	htxd_ecg_manager *p_ecg_manager;

	p_ecg_manager = htxd_get_ecg_manager();

	return p_ecg_manager->ecg_info_list->ecg_start_time;
}



int htxd_get_total_device_count(void)
{
	htxd_ecg_manager *p_ecg_manager;

	p_ecg_manager = htxd_get_ecg_manager();

	return p_ecg_manager->ecg_device_count;
}



int htxd_get_loaded_device_count(void)
{
	htxd_ecg_manager *p_ecg_manager;

	p_ecg_manager = htxd_get_ecg_manager();

	return p_ecg_manager->exer_table_length;
}
