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
/* @(#)63	1.3  src/htx/usr/lpp/htx/bin/htxd/htxd_profile.c, htxd, htxubuntu 8/23/15 23:34:40 */



#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#define PARMS(x) x

#include <cfgclibdef.h>
#include "htxd_profile.h"
#include "htxd.h"
#include "htxd_instance.h"
#include "htxd_util.h"
#include "htxd_trace.h"


#define HTX_PROFILE ".htx_profile"
#define PROFILE_STANZA_SIZE 4096


int htxd_get_max_exer_entries(void)
{
	return (*(htxd_global_instance->p_profile)).max_exerciser_entries;
}

int htxd_get_emc_mode(void)
{
	return (*(htxd_global_instance->p_profile)).emc_run_mode;
}

int htxd_get_slow_shutdown_wait(void)
{
	return (*(htxd_global_instance->p_profile)).slow_shutdown_wait;
}

int htxd_get_max_add_device(void)
{
	return (*(htxd_global_instance->p_profile)).max_added_devices;
}

int htxd_get_hang_monitor_period(void)
{
	return (*(htxd_global_instance->p_profile)).hang_monitor_period;
}

int htxd_get_dr_alarm_value(void)
{
	return (*(htxd_global_instance->p_profile)).dr_alarm_value;
}

char *htxd_get_dr_restart_flag(void)
{
	return (*(htxd_global_instance->p_profile)).dr_restart;
}

int htxd_get_hotplug_signal_delay(void)
{
	return (*(htxd_global_instance->p_profile)).hotplug_signal_delay;
}

int htxd_get_hotplug_restart_delay(void)
{
	return (*(htxd_global_instance->p_profile)).hotplug_restart_delay;
}


/* display profile struct values */
void htxd_display_profile(htxd_profile *p_profile)
{
	if(p_profile == NULL) {
		printf("[DEBUG] : passed profile is NULL, unable to display values\n");
		
	} else {
		printf("[DEBUG] : ====================================================\n");
		printf("[DEBUG] : htxd_display_profile() emc_run_mode = <%d>\n",		p_profile->emc_run_mode);
		printf("[DEBUG] : htxd_display_profile() max_htxerr_size = <%s>\n",		p_profile->max_htxerr_size);
		printf("[DEBUG] : htxd_display_profile() htxerr_wrap = <%s>\n",			p_profile->htxerr_wrap);
		printf("[DEBUG] : htxd_display_profile() max_htxerr_save_size = <%s>\n",	p_profile->max_htxerr_save_size);
		printf("[DEBUG] : htxd_display_profile() max_htxmsg_size = <%s>\n",		p_profile->max_htxerr_size);
		printf("[DEBUG] : htxd_display_profile() htxmsg_wrap = <%s>\n",			p_profile->htxmsg_wrap);
		printf("[DEBUG] : htxd_display_profile() htxmsg_archive = <%s>\n",		p_profile->htxmsg_archive);
		printf("[DEBUG] : htxd_display_profile() max_htxmsg_save_size = <%s>\n",	p_profile->max_htxmsg_save_size);
		printf("[DEBUG] : htxd_display_profile() hang_monitor_period = <%d>\n",		p_profile->hang_monitor_period);
		printf("[DEBUG] : htxd_display_profile() max_added_devices = <%d>\n",		p_profile->max_added_devices);
		printf("[DEBUG] : htxd_display_profile() stress_device = <%s>\n",		p_profile->stress_device);
		printf("[DEBUG] : htxd_display_profile() stress_cycle = <%s>\n",		p_profile->stress_cycle);
		printf("[DEBUG] : htxd_display_profile() max_exerciser_entries = <%d>\n",	p_profile->max_exerciser_entries);
		printf("[DEBUG] : htxd_display_profile() max_ecg_entries = <%d>\n",		p_profile->max_ecg_entries);
		printf("[DEBUG] : htxd_display_profile() auto_start = <%d>\n",			p_profile->auto_start);
		printf("[DEBUG] : ====================================================\n");
	}
	fflush(stdout);
} 


htxd_profile * htxd_create_profile(void)
{
	htxd_profile *new_profile = NULL;
	char trace_string[256];

	new_profile = malloc(sizeof(htxd_profile) );
	if(new_profile == NULL) {
		sprintf(trace_string, "htxd_create_profile: malloc failed with errno <%d>", errno);
		HTXD_TRACE(LOG_ON, trace_string);
		exit(1);
	}
	memset(new_profile, 0, sizeof(htxd_profile) );

	return new_profile;	
}

CFG__SFT * htxd_open_profile(char *profile_file)
{
	CFG__SFT *profile_fd;

	profile_fd = cfgcopsf(profile_file);

	return profile_fd;
}

int htxd_get_profile_autostart(char *p_stanza)
{
	char temp_string[128];

	cfgcskwd("autostart", p_stanza, temp_string);
	htxd_unquote(temp_string);
	if( (temp_string[0] != 'y') && (temp_string[0] == 'Y') ) {
		return 0;
	} else {
		return 1;
	}
}

int htxd_get_profile_run_mode(char *p_stanza)
{
	char temp_string[128];

	cfgcskwd("run_mode", p_stanza, temp_string);
	htxd_unquote(temp_string);
	if( (temp_string[0] == 'r') || (temp_string[0] == 'R') ) {
		return 0;
	} else {
		return 1;
	}
}

int htxd_get_int_profile_parameter(char *parameter_key, char *p_stanza)
{
	int parameter_value;
	char temp_string[128];

	cfgcskwd(parameter_key, p_stanza, temp_string);
	htxd_unquote(temp_string);
	parameter_value = atoi(temp_string);

	return parameter_value;
}


/* read HTX profile file and set the values in profile struct */
int htxd_init_profile(htxd_profile **p_profile)
{

	CFG__SFT *profile_fd;
	char	profile_stanza[PROFILE_STANZA_SIZE];
	char temp_string[300];

	if(*p_profile == NULL) {
		*p_profile = htxd_create_profile();
	}

	/* opening htx profile file */
	sprintf(temp_string, "%s/%s", global_htx_home_dir, HTX_PROFILE);
	profile_fd = htxd_open_profile(temp_string);
	if( (profile_fd != (CFG__SFT *)NULL)  &&
		(cfgcrdsz(profile_fd, profile_stanza, sizeof(profile_stanza), (char *) NULL) == CFG_SUCC) ) {

		/* run mode */
		(*p_profile)->emc_run_mode = htxd_get_profile_run_mode(profile_stanza);

		/* max_err_file */
		cfgcskwd("max_err_file", profile_stanza, (*p_profile)->max_htxerr_size);
		htxd_unquote((*p_profile)->max_htxerr_size);

		/* err_wrap */
		cfgcskwd("err_wrap", profile_stanza, (*p_profile)->htxerr_wrap);
		htxd_unquote((*p_profile)->htxerr_wrap);
		if( ( (*p_profile)->htxerr_wrap[0] != 'y') && ( (*p_profile)->htxerr_wrap[0] != 'Y') ) {
			strcpy( (*p_profile)->htxerr_wrap, "no");
		} else {
			strcpy( (*p_profile)->htxerr_wrap, "yes");
		}

		/* err_wrap */
		cfgcskwd("msg_archive", profile_stanza, (*p_profile)->htxmsg_archive);
		htxd_unquote((*p_profile)->htxmsg_archive);
		if( ( (*p_profile)->htxmsg_archive[0] != 'y') && ( (*p_profile)->htxmsg_archive[0] != 'Y') ) {
			strcpy( (*p_profile)->htxmsg_archive, "no");
		} else {
			strcpy( (*p_profile)->htxmsg_archive, "yes");
		}

		/* max_err_save */
		cfgcskwd("max_err_save", profile_stanza, (*p_profile)->max_htxerr_save_size);
		htxd_unquote((*p_profile)->max_htxerr_save_size);

		/* max_msg_file */
		cfgcskwd("max_msg_file", profile_stanza, (*p_profile)->max_htxmsg_size);
		htxd_unquote((*p_profile)->max_htxmsg_size);

		/* msg_wrap */
		cfgcskwd("msg_wrap", profile_stanza, (*p_profile)->htxmsg_wrap);
		htxd_unquote((*p_profile)->htxmsg_wrap);
		if( ( (*p_profile)->htxmsg_wrap[0] != 'y') && ( (*p_profile)->htxmsg_wrap[0] != 'Y') ) {
			strcpy( (*p_profile)->htxmsg_wrap, "no");
		} else {
			strcpy( (*p_profile)->htxmsg_wrap, "yes");
		}

		/* autostart */
		(*p_profile)->auto_start = htxd_get_profile_autostart(profile_stanza);
		/*cfgcskwd("autostart", profile_stanza, (*p_profile)->auto_start);
		htxd_unquote((*p_profile)->auto_start);
		if( ( (*p_profile)->auto_start[0] != 'y') && ( (*p_profile)->auto_start[0] != 'Y') ) {
			strcpy( (*p_profile)->auto_start, "no");
		} else {
			strcpy( (*p_profile)->auto_start, "yes");
		}*/

		/* stress_dev */
		cfgcskwd("stress_dev", profile_stanza, (*p_profile)->stress_device);
		htxd_unquote((*p_profile)->stress_device);
		if( strlen((*p_profile)->stress_device) == 0) {
			strcpy((*p_profile)->stress_device, "/dev/null");
		}

		/* stress_cycle */
		cfgcskwd("stress_cycle", profile_stanza, (*p_profile)->stress_cycle);
		htxd_unquote((*p_profile)->stress_cycle);
		if( strlen((*p_profile)->stress_cycle) == 0) {
			strcpy((*p_profile)->stress_cycle, "0");
		}

		/* max_msg_save */
		cfgcskwd("max_msg_save", profile_stanza, (*p_profile)->max_htxmsg_save_size);
		htxd_unquote((*p_profile)->max_htxmsg_save_size);

		/* hang_mon_period */
		(*p_profile)->hang_monitor_period = htxd_get_int_profile_parameter("hang_mon_period", profile_stanza);

		/* slow_shutd_wait */
		(*p_profile)->slow_shutdown_wait = htxd_get_int_profile_parameter("slow_shutd_wait", profile_stanza);

		/* max_added_devices : extra devices can be added in future */
		(*p_profile)->max_added_devices = htxd_get_int_profile_parameter("max_added_devices", profile_stanza);

		/* max_exerciser_entries */
		(*p_profile)->max_exerciser_entries = htxd_get_int_profile_parameter("max_exer_entries", profile_stanza);

		/* max_ecg_entries */
		(*p_profile)->max_ecg_entries =  htxd_get_int_profile_parameter("max_ecgs", profile_stanza);

#ifdef __HTXD_DR__
		/* dr_alarm_value */
		(*p_profile)->dr_alarm_value =  htxd_get_int_profile_parameter("dr_alarm_value", profile_stanza);

		/* dr_restart */
		cfgcskwd("dr_restart", profile_stanza, (*p_profile)->dr_restart);
		htxd_unquote((*p_profile)->dr_restart);
		if( ( (*p_profile)->dr_restart[0] != 'y') && ( (*p_profile)->dr_restart[0] != 'Y') ) {
			strcpy( (*p_profile)->dr_restart, "no");
		} else {
			strcpy( (*p_profile)->dr_restart, "yes");
		}
#endif

#ifdef __HTX_LINUX__
		/* hot plug signal delay */
		(*p_profile)->hotplug_signal_delay =  htxd_get_int_profile_parameter("hotplug_signal_delay", profile_stanza);

		/* hotplug restart delay */
		(*p_profile)->hotplug_restart_delay =  htxd_get_int_profile_parameter("hotplug_restart_delay", profile_stanza);
#endif

		/* close htx profile file */
		if (cfgcclsf(profile_fd) != CFG_SUCC) {
			sprintf(temp_string, "htxd_init_profile: cfgcclsf not returned CFG_SUCC");
			HTXD_TRACE(LOG_ON, temp_string);
			exit(1);
		}

	} else {
		sprintf(temp_string, "Error : failed to open HTX profile file");
		HTXD_TRACE(LOG_ON, temp_string);
	}
	

	return 0;
}


int htxd_reload_htx_profile(htxd_profile **p_profile)
{

	int result = 0;

	if(*p_profile != NULL) {
		free(*p_profile);
		*p_profile = NULL;
	}

	result = htxd_init_profile(p_profile);
	htxd_execute_shell_profile();

	return result;
}


/* get max exterciser entries from pofile struct */
int htxd_get_profile_max_exer_entries(void)
{
	htxd *p_htxd_instance;
	int max_exer_entries;

	p_htxd_instance = htxd_get_instance();
	max_exer_entries = p_htxd_instance->p_profile->max_exerciser_entries;

	return max_exer_entries;
}
