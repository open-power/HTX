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

/************************************************************/
/*  Filename:   hxestorage.c                                */
/*  contains main and other functions of the program        */
/************************************************************/

#include "io_oper.h"

time_t time_mark;
pthread_t hang_monitor_thread, sync_cache_th, stats_update_th;

pthread_attr_t thread_attrs_detached;    /* threads created detached */
pthread_cond_t  create_thread_cond_var, do_oper_cond_var;
pthread_cond_t threads_finished_cond_var;
pthread_mutex_t thread_create_mutex, cache_mutex, stats_mutex, fencepost_mutex, dump_mutex;
#ifndef __HTX_LINUX__
pthread_mutex_t log_mutex;
#endif

struct device_info dev_info;

int total_BWRC_threads, num_non_BWRC_threads;
int free_BWRC_th_mem_index = 0;
int threshold = DEFAULT_THRESHOLD, hang_time = DEFAULT_HANG_TIME;
int max_thread_cnt = 0; 	/* Highest thread count defined in rule file stanza */
int sync_cache_flag = 0, randomize_sync_cache = 1;
int discard_enabled = 0, enable_state_table = NO;
int eeh_retries = 1, turn_attention_on = 0, read_rules_file_count;
volatile char exit_flag = 'N', signal_flag = 'N', int_signal_flag = 'N';
char misc_run_cmd[256] = "", run_on_misc = 'N';
struct thread_context *non_BWRC_threads_mem = NULL, *BWRC_threads_mem = NULL;
struct htx_data data, global_htx_d;

char *mmap_ptr= NULL;
int mmap_fd, mmap_offset = 0;

#ifdef __CAPI_FLASH__
oper_fptr oper_list[TESTCASE_TYPES][MAX_OPER_COUNT] = {{cflsh_read_operation, cflsh_write_operation, compare_buffers, cflsh_unmap_operation,NULL, NULL},
                                                       {cflsh_aread_operation, cflsh_awrite_operation, NULL, NULL, NULL, NULL},
                                                      };
int lun_type = UNDEFINED;
char capi_device[MAX_STR_SZ];
size_t chunk_size = DEFAULT_CHUNK_SIZE;
volatile chunk_id_t shared_lun_id = NULL_CHUNK_ID;
#else
oper_fptr oper_list[TESTCASE_TYPES][MAX_OPER_COUNT] = {{&read_disk, &write_disk, &compare_buffers, &discard, NULL, NULL},
                                                      {&read_async_disk, &write_async_disk, &cmp_async_disk, NULL, NULL, NULL},
                                                      {&read_passth_disk, &write_passth_disk, &verify_passth_disk, NULL, NULL, NULL},
                                                      {&read_cache, &write_cache, &compare_cache, NULL, NULL, NULL}
                                                     };
#endif


/********************/
/*  Main function   */
/********************/
int main(int argc, char *argv[])
{
    int i, err_no, rule_no, rc = 0;
    int wait_count = DEFAULT_WAIT_COUNT, skip_flag;
    char *kdblevel = NULL, *eeh_env = NULL, *ptr = NULL;
    struct ruleinfo *current_ruleptr;
    char msg[MAX_TEXT_MSG], script_path[64], cmd_str[128];
    struct thread_context *current_tctx;
    char file_name[128];

    struct sigaction sigvector, sigdata;
    static sigset_t  sigmask;

    /****************************************/
    /**     Register Signal handlers        */
    /****************************************/
    sigemptyset((&sigvector.sa_mask)); /* empty the mask */
    sigvector.sa_flags = 0;
    sigvector.sa_handler = (void (*)(int)) SIGTERM_hdl;
    sigaction(SIGTERM, &sigvector, (struct sigaction *) NULL); /* set signal handler for SIGTERM */


    sigprocmask(0, NULL, &sigmask); /* get current signal mask     */
    sigdata.sa_flags = SA_RESTART; /* restart on signal calls     */
    sigdata.sa_mask = sigmask;  /* set the signal mask */

    sigdata.sa_handler = (void(*)(int))sig_function;
    sigaction(SIGUSR1, &sigdata, NULL); /* call when recv sig 30 */

    sigdata.sa_handler = (void(*)(int))int_sig_function;
    sigaction(SIGINT, &sigdata, NULL); /* set signal handler for SIGINT */

    /*********************************************/
    /** Initialize thread attributes and mutex  **/
    /*********************************************/
    pthread_attr_init(&thread_attrs_detached);
    pthread_attr_setdetachstate(&thread_attrs_detached, PTHREAD_CREATE_DETACHED);

    pthread_mutex_init(&thread_create_mutex, DEFAULT_MUTEX_ATTR_PTR);
    pthread_mutex_init(&cache_mutex, DEFAULT_MUTEX_ATTR_PTR);
    pthread_mutex_init(&fencepost_mutex, DEFAULT_MUTEX_ATTR_PTR);
    pthread_mutex_init(&stats_mutex, DEFAULT_MUTEX_ATTR_PTR);
    pthread_mutex_init(&dump_mutex, DEFAULT_MUTEX_ATTR_PTR);
#ifndef __HTX_LINUX__
    pthread_mutex_init(&log_mutex, DEFAULT_MUTEX_ATTR_PTR);
#endif

    pthread_cond_init(&create_thread_cond_var, DEFAULT_COND_ATTR_PTR);
    pthread_cond_init(&do_oper_cond_var, DEFAULT_COND_ATTR_PTR);
    pthread_cond_init(&threads_finished_cond_var, DEFAULT_COND_ATTR_PTR);

    bzero(&dev_info, sizeof(dev_info));
    dev_info.cont_on_misc = UNINITIALIZED;

    /************************************************/
    /*******   Read command line arguments    *******/
    /************************************************/
    memset(&data, 0, sizeof(struct htx_data));
    strcpy(data.HE_name, argv[0]);
    strcpy(data.sdev_id, argv[1]);
    strcpy(dev_info.dev_name, argv[1]);
    strcpy(data.run_type, argv[2]);
    strcpy(dev_info.rules_file_name, argv[3]);

    hxfupdate(START, &data);
    sprintf(msg, "%s %s %s %s \n", data.HE_name, data.sdev_id, data.run_type, dev_info.rules_file_name);
    user_msg(&data, 0, 0, INFO, msg);

    /* globla_htx_d will be used by stats thread. So, this need to be updated
     * from data.
     */
     memcpy(&global_htx_d, &data, sizeof(struct htx_data));

    /*************************************************************************/
    /* Sanity check:                                                         */
    /* Before we open the device, check to see if it is a member of a volume */
    /* group which is currently defined in the ODM.  This check prevents     */
    /* accidentally clobbering a disk which was added to a volume group      */
    /* after logging on as htx.                                              */
    /* If check_disk returns < 0, then the device is OK to exercise.         */
    /*************************************************************************/

    ptr = getenv("HTXSCRIPTS");
    if (ptr != NULL) {
		strcpy(script_path, ptr);
    } else {
        sprintf(msg, "HTX env. is not set. Hence exiting.\nPlease setup the env. and re-run the test");
        user_msg(&data, 0, 0, INFO, msg);
        exit(1);
    }

#ifndef __HTX_LINUX__
    if ( (rc = check_disk(data.sdev_id, msg, sizeof(msg))) == 0) {
        user_msg(&data, 0, 0, HARD, msg);
        exit(126);
    } else if (rc > 0 ) {
        sprintf(msg, "ODM or LVM error in check_disk, rc = %d\n%s", rc, msg);
        user_msg(&data, 0, 0, HARD, msg);
        exit(1);
    }
#else
    sprintf(cmd_str, "%scheck_disk %s", script_path, data.sdev_id);
    rc = system(cmd_str);
    if (WIFEXITED(rc)) {
        if (WEXITSTATUS(rc) == 1) {
            sprintf(msg, "Disk either has partitions OR is used by the system. So, can not run exerciser on it. Hence exiting.");
            user_msg(&data, 0, 0, INFO, msg);
            exit(1);
        }
    } else {
        sprintf(msg, "command did not completed properly. Hence exiting.");
        user_msg(&data, 0, 0, INFO, msg);
        exit(1);
    }
#endif

    /****************************************************/
    /****** Get hostname for machine running code *******/
    /****************************************************/
    rc = gethostname(dev_info.hostname, 16);
    if ( rc != 0 ) {
        sprintf(msg, "Unable to get hostname for this run!, errno = %d \n", errno);
        user_msg(&data, 0, 0, INFO, msg);
    }

    /*****************************************************/
    /******* Get HTXKDBLEVEL and EEH env. variable *******/
    /*****************************************************/
    dev_info.crash_on_miscom = 1;
    kdblevel = getenv("HTXKDBLEVEL");
    if (kdblevel != NULL) {
        if (!(atoi(kdblevel))) {
            dev_info.crash_on_miscom = 0;
        }
    }
    eeh_env = getenv("HTXEEH");
    if (eeh_env != NULL) {
        if (atoi(eeh_env)) {
            ptr = getenv("HTXEEHRETRIES");
            if (ptr != NULL) {
                eeh_retries = atoi(getenv("HTXEEHRETRIES"));
			}
        } else if((strcmp(data.run_type, "REG") == 0) && data.p_shm_HE->cont_on_err) {
            /* If User specifies COE in mdt file, then need to make sure all
             * read/write call are successfull. If we see ocassional error, retry.
             */
            eeh_retries = DEFAULT_EEH_RETRIES;
        }
    }

    /* Create mmaped dump file */
    sprintf (file_name, "%s/IO_details_dump.log", data.htx_exer_log_dir);
    mmap_fd = open (file_name, O_RDWR | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
    if (mmap_fd == -1) {
        sprintf(msg, "Could not open the dump file. errno: %d\n", errno);
        user_msg(&data, 0, 0, HARD, msg);
    }
    if (lseek (mmap_fd, MMAP_FILE_SIZE, SEEK_SET) == -1) {
        perror("lseek");
    }
    rc = write(mmap_fd, "", 1);
    close(mmap_fd);

    mmap_fd = open (file_name, O_RDWR);
    if (mmap_fd == -1) {
        sprintf(msg, "Could not open the dump file. errno: %d\n", errno);
        user_msg(&data, 0, 0, HARD, msg);
    }
    /* Do the mmap of the file */
    mmap_ptr = (char *) mmap( NULL, MMAP_FILE_SIZE, PROT_READ | PROT_WRITE, MAP_FILE | MAP_SHARED, mmap_fd, 0);
    if (mmap_ptr == (void *) -1) {
        sprintf(msg, "mmap failed. errno: %d\n", errno);
        user_msg(&data, 0, 0, HARD, msg);
    }
    close(mmap_fd);

    /****************************************************************************/
	/*   For CAPI flash, Device given as input is of format /dev/rhdisk[0-1]*.N */
    /*   on AIX and sd[a-z][a-z]*.N on Linux. Actual device would be            */
    /*   /dev/rhdisk[0-1] OR /dev/sd[a-z][a-z, 0-1]* and our invocation count   */
    /*   would be N. Extract device name from argv[1]                           */
	/****************************************************************************/

#ifdef __CAPI_FLASH__
	char file[128] = "/tmp/test_lun_mode";
    FILE * fp ;
	uint32_t num_instance = 0;
	char buf[128];

    char device[MAX_STR_SZ], intance[MAX_STR_SZ], input_dev[MAX_STR_SZ], temp[MAX_STR_SZ];
    int str_len, indx=0;
    strcpy(input_dev, basename(data.sdev_id));
    str_len = strlen(input_dev);

	rc = cblk_init(NULL, 0);
	if (rc != 0) {
       sprintf(msg, "cblk_init returned error. rc: %d.", rc);
       user_msg(&data, 0, 0, HARD, msg);
       exit(1);
    }

    for(i = 0; i <= str_len; i++ ) {
        temp[indx]=input_dev[i];
        if(input_dev[i] == '.' ) {
            strncpy(device, temp, indx);
            device[indx] = '\0';
            indx = 0;
            memset(temp, 0, MAX_STR_SZ);
        } else  if (input_dev[i] == '\0') {
            strcpy(intance, temp);
        } else {
            indx++;
        }
    }

	if(device[0] == 'r') {
    	strncpy(dev_info.diskname, &device[1], MAX_STR_SZ);
	} else {
		strncpy(dev_info.diskname, &device[0], MAX_STR_SZ);
	}
	sprintf(capi_device,"/dev/%s",&device[0]);
    printf("input_dev=%s, device=%s, intance=%s, capi_device=%s \n", input_dev, device, intance, capi_device);
    fflush(stdout);

	rc = access(file, F_OK);
    if(rc == -1 ) {
    #ifdef __HTX_LINUX__
        /* In case of Linux, if file does not exist, we run in VLUN mode.
         * Hence, setting the params for VLUN mode.
         */
        lun_type = CBLK_OPN_VIRT_LUN;
        dev_info.maxblk = (unsigned long long)VLUN_TOTALBLOCKS;
        dev_info.blksize = (unsigned int)VLUN_BLKSIZE;
    #else
        /* In case of AIX, if file does not exist, means we are testing in Legacy mode.
         * hxestorage should run on this device instead of hxesurelock. Hence, exiting.
         */
		sprintf(msg, "File: %s, doesnot exists!! this exerciser should not have been envoked. \n", file);
		user_msg(&data, 0, 0, HARD, msg);
		return(rc);
    #endif
	} else {
        /************************************************/
	    /* Get content of file : "/tmp/test_lun_mode"   */
	    /************************************************/
	    sprintf(cmd_str, "cat %s", file);
        fp = popen(cmd_str, "r");
        if (fp == NULL) {
            sprintf(msg, "popen failed while reading file=%s with errno=%d. Setting Virtual LUN mode. \n", file, errno);
            user_msg(&data, 0, 0, INFO, msg);
	    	num_instance = 2;
        } else {
            if (fgets(buf, 128, fp) == NULL) {
                sprintf(msg, "fgets failed while reading from file = %s, errno=%d. Setting Virtual LUN mode. \n \n", file, errno);
                user_msg(&data, 0, 0, INFO, msg);
                pclose(fp);
	    		num_instance = 2;
            } else {
                pclose(fp);
                num_instance = atoi(buf);
            }
        }

	    if(num_instance == 1) {
	    	/***********************************************************/
            /* File exists with value=1, we are testing PLUNs. We can  */
            /* query IOCINFO on PLUNs for total blocks and block size. */
            /***********************************************************/
            lun_type            = CBLK_OPN_PHY_LUN;
        #ifndef __HTX_LINUX__
            rc = get_disk_info(&data, capi_device);
            if ( rc != 0) {
                exit(1);
            }
            /****************************************************/
            /* For PLUN chunk_size is same as num blocks on disk*/
            /****************************************************/
            chunk_size = dev_info.maxblk + 1;
        #else
            chunk_id_t chunk_id = NULL_CHUNK_ID;
        #ifdef __HTX_LINUX__
            chunk_attrs_t attr;
        #endif

            chunk_id = cblk_open(capi_device, MAX_THREADS, O_RDWR, 0, 0);
            if(chunk_id == NULL_CHUNK_ID) {
                sprintf(msg, "main : Cannot get a valid chunk_id, capi_device = %s, errno = %d \n", capi_device, errno);
                user_msg(&data, errno, 0, HARD, msg);
                exit(1);
	        }
            rc = cblk_get_lun_size(chunk_id, (unsigned long *)&dev_info.maxblk, 0);
            if(rc ) {
                sprintf(msg, "cblk_get_size failed with rc = %d, errno = %d \n", rc, errno);
                user_msg(&data, errno, 0, HARD, msg);
                return(1);
            }
        #ifdef __HTX_LINUX__
            rc = cblk_get_attrs (chunk_id, &attr, 0);
            if (rc) {
                sprintf(msg, "cblk_get_attrs failed with rc = %d, errno = %x \n", rc, errno);
                user_msg(&data, errno, 0, HARD, msg);
                exit(1);
            }

            if (attr.flags1 & CFLSH_ATTR_UNMAP) {
                discard_enabled = 1;
            }
        #endif
            rc = cblk_close(chunk_id, CBLK_SCRUB_DATA_FLG);
            if(rc) {
                sprintf(msg, "cblk_close failed with rc = %d, errno = %x \n", rc, errno);
                user_msg(&data, errno, 0, HARD, msg);
                exit(1);
            }
            chunk_id = NULL_CHUNK_ID;
            /****************************************************/
            /* For PLUN chunk_size is same as num blocks on disk*/
            /****************************************************/
            chunk_size = dev_info.maxblk;
            dev_info.blksize = (unsigned int)VLUN_BLKSIZE;
        #endif
	    } else {
            if (num_instance == 0) {
            #ifdef __HTX_LINUX__
                 /* in case of LInux, If value in file is 0, means we are testing VLUN */
                dev_info.maxblk = (unsigned long long)VLUN_TOTALBLOCKS;
                dev_info.blksize = (unsigned int)VLUN_BLKSIZE;
                lun_type = CBLK_OPN_VIRT_LUN;
            #else
                /* In case of AIX, value 0 means, we need to run in LEGACY mode. So error out */
                sprintf(msg, "File: %s, exists but value = %d, this exerciser should not have been envoked. \n", file, num_instance);
                user_msg(&data, 0, 0, HARD, msg);
                return(rc);
            #endif
            } else {
                /********************************************************/
	            /* File exists with value > 1, we are testing VLUNS.    */
	    	    /* VLUNs have capability to dynamically change size.    */
	    	    /* So user request for it through chunk_size rules parm.*/
	    	    /********************************************************/
	 	        dev_info.maxblk 	= (unsigned long long)VLUN_TOTALBLOCKS;
	            dev_info.blksize 	= (unsigned int)VLUN_BLKSIZE;
		        lun_type 			= CBLK_OPN_VIRT_LUN;
	        }
        }
    }
#else
    /***************************************************/
    /****   Get disk info using IOCINFO ioctl.      ****/
    /****   This will update maxblk and blksize.    ****/
    /***************************************************/
    rc = get_disk_info(&data, data.sdev_id);
    if ( rc != 0) {
        exit(1);
    }
#endif

    /***************************************************/
    /********           Read rule file          ********/
    /***************************************************/
    rc = read_rf(&data, dev_info.rules_file_name, dev_info.maxblk, dev_info.blksize);
    if (rc != 0) {
        sprintf(msg, "Check HTXERR log for a listing of errors with rule file reading!\n");
        user_msg(&data, 0, 0, HARD, msg);
        exit(1);
    }
    segment_table_init();

    /* if run_on_misc is set to YES and misc_run_cmd is null. {Populate the default script to run*/
    if (run_on_misc == 'Y' && strcmp(misc_run_cmd, "") == 0) {
        sprintf(misc_run_cmd, "%s/storage_miscom_pre_kdb_script", script_path);
    }

    time_mark = time(0); /* set time mark for comparison */
    if (enable_state_table == YES) {
        rc = initialize_state_table(&data, data.sdev_id);
        if (rc == -1) {
			exit(1);
	    }
    }

#ifdef __HTX_LINUX__
    /* Check if write cahce is enabled if sync_cache is set to yes in any
     * of the rule. This will set dev_info.write_cache to 1 if enabled,
     * otherwise 0.
     */
    dev_info.write_cache = 0;
    if (sync_cache_flag == 1) {
        dev_info.write_cache = check_write_cache (&data);
        /* Create a separate thread to issue CACHE_SYNC ioctl at regular interval */
        if (dev_info.write_cache == 1 && randomize_sync_cache == 1) {
            rc = pthread_create(&sync_cache_th, &thread_attrs_detached, (void *(*)(void *))sync_cache_thread, (void *)(&data));
            if (rc != 0) {
                sprintf(msg, "pthread_create failed for sync_cache threas. errno. set is: %d\n", rc);
                user_msg(&data, rc, 0, HARD, msg);
                exit(1);
		    }
        } else if (dev_info.write_cache == 0) {
            sprintf(msg, "Write cache is is not enabled. Will not run sync_cache thread.");
            user_msg(&data, 0, 0, INFO, msg);
        }
        sprintf(msg, "write cache is : %d\n", dev_info.write_cache);
        user_msg(&data, 0, 0, INFO, msg);
    }
#endif

    /*****************************************************************/
    /* Allocate memory for BWRC threads. Since lifetime of BWRC      */
    /* threads can span across multiple stanzas. So, we pre-allocate */
    /* memory for all BWRC threads defined in various stanzas in the */
    /* rulefile. Information of BWRC threads of various stanzas once */
    /* updated here will be maintained till end of the exerciser.    */
    /*****************************************************************/
    if (total_BWRC_threads != 0) {
        BWRC_threads_mem = (struct thread_context *) malloc(total_BWRC_threads * sizeof(struct thread_context));
        if (BWRC_threads_mem == NULL) {
            err_no = errno;
            sprintf(msg, "malloc failed(with errno: %d) while allocating memory for BWRC thread context.\n", errno);
            user_msg(&data, err_no, 0, HARD, msg);
            exit(1);
        }
        initialize_threads(BWRC_threads_mem, total_BWRC_threads);
        for (rule_no = 0; rule_no < num_rules_defined; rule_no++) {
            current_ruleptr = &rule_list[rule_no];
            if (current_ruleptr->num_BWRC_threads == 0) {
                continue;
            } else {
                populate_BWRC_thread_context(&data, current_ruleptr, BWRC_threads_mem);
            }
        }
    }

    /*********************************************************************/
    /****** run hang_monitor thread to keep a check of any HUNG IO  ******/
    /*********************************************************************/
    rc = pthread_create(&(hang_monitor_thread), &thread_attrs_detached, (void *(*)(void *))hang_monitor, (void *)(&data));
    if (rc != 0) {
        sprintf(msg, "pthread_create failed for hang_monitor thread. errno set is: %d\n", rc);
        user_msg(&data, rc, 0, HARD, msg);
        /* Free BWRC memory also if allocated */
        if (BWRC_threads_mem != NULL) {
            free (BWRC_threads_mem);
            BWRC_threads_mem = NULL;
        }
        exit(1);
    }

    /*******************************************************/
    /******     Create stats updation thread         *******/
    /*******************************************************/
    rc = pthread_create(&stats_update_th, &thread_attrs_detached, (void *(*)(void *))stats_update_thread, NULL);
    if (rc != 0) {
        sprintf(msg, "pthread_create failed for stats update thread. errno set is: %d\n", rc);
        user_msg(&data, rc, 0, HARD, msg);
        /* Free BWRC memory also if allocated */
        if (BWRC_threads_mem != NULL) {
            free (BWRC_threads_mem);
            BWRC_threads_mem = NULL;
        }
        exit(1);
    }

    /******************************************************/
    /*******    Start processing of stanza          *******/
    /******************************************************/
    read_rules_file_count = 1;
    do {
        current_ruleptr = &rule_list[0];
        for (rule_no = 0; rule_no < num_rules_defined; rule_no++) {
            /* DPRINT("%s:%d - Running stanza: %d\n", __FUNCTION__, __LINE__, rule_no + 1); */
            data.test_id = rule_no + 1;
            HTX_STATS_UPDATE (UPDATE , &data);

            skip_flag = 0;
            /******************************************************************/
            /***    Check rule parameter to see if need to skip this stanza ***/
            /******************************************************************/
            if ( current_ruleptr->repeat_pos > 0 ) {
                if ((read_rules_file_count == 1)                                ||
                    ((read_rules_file_count % current_ruleptr->repeat_pos) !=0) ||
                    ((current_ruleptr->repeat_pos == 1)                         &&
                    (!(read_rules_file_count & 1)))
                )  {
                    skip_flag = 1;
                }
            } else if ( current_ruleptr->repeat_neg > 0 ) {
                if (((read_rules_file_count != 1)                                   &&
                    ((read_rules_file_count % current_ruleptr->repeat_neg) != 0))   ||
                    ((current_ruleptr->repeat_neg == 1)                             &&
                    (!(read_rules_file_count & 1))                                  &&
                    (read_rules_file_count != 1))) {
                    skip_flag = 1;
                }
            }

            /****************************************************/
            /**  If not skipping the stanza, run the testcase  **/
            /****************************************************/
            if (skip_flag != 1) {
                if (current_ruleptr->oper[0][0] == 'S' || current_ruleptr->oper[0][0] == 's') {
                    usleep(current_ruleptr->sleep);
                } else if (strcasecmp(current_ruleptr->oper[0], "XCMD") == 0) {
                    rc = run_cmd(&data, current_ruleptr->cmd_list);
                } else {
                    /************************************************************************/
                    /****           Allocate memory for non BWRC threads context        *****/
                    /************************************************************************/
                    num_non_BWRC_threads = current_ruleptr->num_threads - current_ruleptr->num_BWRC_threads;
                    if (num_non_BWRC_threads) {
                        non_BWRC_threads_mem = (struct thread_context *) malloc (num_non_BWRC_threads * sizeof(struct thread_context));
                        if (non_BWRC_threads_mem == NULL) {
                            err_no = errno;
                            sprintf(msg, "malloc failed(with errno: %d) while allocating memory for thread context.\n", errno);
                            user_msg(&data, err_no, 0, HARD, msg);
                            /* Free BWRC memory also if allocated */
                            if (BWRC_threads_mem != NULL) {
                                free (BWRC_threads_mem);
                                BWRC_threads_mem = NULL;
                            }
                            exit(1);
                        }
                        initialize_threads(non_BWRC_threads_mem, num_non_BWRC_threads);

                        /**********************************************/
                        /* Populate thread context for all non BWRC   */
                        /* threads defined in current stanza.         */
                        /**********************************************/
                        rc = populate_thread_context(&data, current_ruleptr, BWRC_threads_mem, non_BWRC_threads_mem);
                    }

                    /**************************************************************/
                    /*** Spawn threads to execute testcase. First, BWRC threads ***/
                    /*** will be created for current stanza, if defined any.    ***/
                    /*** After that, non BWRC threads will be created.          ***/
                    /**************************************************************/
                    rc = pthread_mutex_lock(&thread_create_mutex);
                    if (rc) {
                        sprintf(msg, "Mutex lock failed in MAIN, rc = %d\n", rc);
                        user_msg(&data, rc, 0, HARD, msg);
                        exit(1);
                    }
                    for (i = 0; i < current_ruleptr->num_BWRC_threads; i++) {
                        current_tctx = &(BWRC_threads_mem[current_ruleptr->BWRC_th_mem_index + i]);

                        /* Need to check if current BWRC thread status is 'F' (i.e. Finished), then only
                         * need to create the thread. Otherwise, skip it.
                         */
                        if (lba_fencepost[current_tctx->fencepost_index].status == 'F') {
                            rc = pthread_create(&(current_tctx->tid), &thread_attrs_detached,
                                            (void *(*)(void *)) execute_thread_context, (void *) current_tctx);
                            if (rc) {
                                sprintf(msg, "rc %d, errno %d from main(): pthread_create for BWRC threads no: %d",
                                        rc, errno, i);
                                user_msg(&data, rc, 0, HARD, msg);
                                exit(rc);
                            }
                            rc = pthread_cond_wait(&create_thread_cond_var, &thread_create_mutex);
                            if (rc) {
                                sprintf(msg, "Cond wait failed in MAIN for BWRC thread, rc = %d\n", rc);
                                user_msg(&data, rc, 0, HARD, msg);
                                exit(rc);
                            }
                            BWRC_threads_running++;
                        } else {
                            continue;
                        }
                    }
                    for (i=0; i < num_non_BWRC_threads; i++) {
                        current_tctx = &(non_BWRC_threads_mem[i]);
                        current_tctx->th_num = i;
                        rc = pthread_create(&(current_tctx->tid), &thread_attrs_detached,
                                    (void *(*)(void *)) execute_thread_context, (void *) current_tctx);
                        if (rc) {
                            sprintf(msg, "rc %d, errno %d from main(): pthread_create", rc,errno);
                            user_msg(&data, rc, 0, HARD, msg);
                            exit(rc);
                        }
                        rc = pthread_cond_wait(&create_thread_cond_var, &thread_create_mutex);
                        if (rc) {
                            sprintf(msg, "Cond wait failed in MAIN, rc = %d\n", rc);
                            user_msg(&data, rc, 0, HARD, msg);
                            exit(rc);
                        }
                        non_BWRC_threads_running++;
                    }

                    /**********************************************************************/
                    /******      Send broadcast to all threads to start executing   *******/
                    /**********************************************************************/
                    rc = pthread_cond_broadcast(&do_oper_cond_var);
                    if ( rc ) {
                        sprintf(msg,"pthread_cond_broadcast failed for do_oper_cond_var. rc = %d\n", rc);
                        user_msg(&data, rc, 0, HARD, msg);
                        exit(1);
                    }

                    /*************************************************************/
                    /****   wait for threads to finish if non BWRC stanza   ******/
                    /*************************************************************/
                    if (current_ruleptr->is_only_BWRC_stanza == 'N') {
                        rc = pthread_cond_wait(&threads_finished_cond_var, &thread_create_mutex);
                        if (rc) {
                            sprintf(msg, "Cond wait failed in MAIN for threads_finished condition variable.rc = %d\n", rc);
                            user_msg(&data, rc, 0, HARD, msg);
                            exit(rc);
                        }
                    }
                    rc = pthread_mutex_unlock(&thread_create_mutex);
                    if (rc) {
                        sprintf(msg, "Mutex unlock failed in MAIN, rc = %d\n", rc);
                        user_msg(&data, rc, 0, HARD, msg);
                        exit(rc);
                    }

                    /***********************************************************/
                    /********       Clean up thread context memory      ********/
                    /***********************************************************/
                    if (non_BWRC_threads_mem != NULL) {
                        free(non_BWRC_threads_mem);
                        non_BWRC_threads_mem = NULL;
                    }
                }
            } else {
                sprintf(msg, "Rule %s Has Been SKIPPED......\n", current_ruleptr->rule_id);
                user_msg(&data, 0, 0, INFO, msg);
            }
            current_ruleptr++;

            if (exit_flag == 'Y' || int_signal_flag == 'Y' || signal_flag == 'Y') {
                break;
            }
            /* DPRINT("%s:%d - Completed stanza: %d\n", __FUNCTION__, __LINE__, rule_no + 1); */
        } /* End rule_no < num_rules_defined for loop */

        if (signal_flag == 'Y') {
			/*************************************************************************/
            /* Below code looks to see if it has received a signal 30 from the user. */
            /* This is to see if the user has made a change to the rules file. The   */
            /* program will reread the rules file and start processing again.        */
            /*************************************************************************/
            total_BWRC_threads = 0;
            free_BWRC_th_mem_index = 0;
            num_rules_defined = 0;
            signal_flag = 'N';
            rc = read_rf(&data, dev_info.rules_file_name, dev_info.maxblk, dev_info.blksize);
            if ( rc != 0 ) {
                sprintf(msg, "Check the HTXERR log for error information on rule file reading.\n");
                user_msg(&data, 0, 0, HARD, msg);
                return (-1);
            }
        }
        sprintf(msg, "Pass #%d, rule file %s completed.\nCollision count = %d.", read_rules_file_count,
                dev_info.rules_file_name, collisions);
        user_msg(&data, 0, 0, INFO, msg);
        HTX_STATS_UPDATE (FINISH, &data);
        read_rules_file_count++;
    } while ((strcmp(data.run_type, "REG") == 0) && (exit_flag == 'N') && (int_signal_flag == 'N'));

    /****************************************************/
    /** Set the exit flag and wait for any BWRC thread **/
    /** if  running                                    **/
    /****************************************************/
    exit_flag = 'Y';
#ifdef __CAPI_FLASH__
    /* Wait for the thread (i.e. the thread for which open_flag was set) to close the LUN. */
    while (shared_lun_id != NULL_CHUNK_ID) {
        sleep(5);
    }
#endif
    while (BWRC_threads_running != 0 && wait_count > 0) {
        sleep(5);
        wait_count--;
    }

    /********************************************/
    /*****      Cleanup threads memory      *****/
    /********************************************/

    if (BWRC_threads_running != 0) {
        sprintf(msg, "Going to cleanup memory. Since some BWRC threads are still running, ignore if any core is generated.");
        user_msg(&data, 0, 0, INFO, msg);
    }
    cleanup_threads_mem();
    munmap(mmap_ptr, MMAP_FILE_SIZE);

    /* If enable_state_table flag is YES, SYNC the state table (i.e. update metadata on disk) */
    if (enable_state_table == YES) {
        rc = sync_state_table(&data, data.sdev_id);
        if (rc) {
			sprintf(msg, "Failed to update state table.\n");
            user_msg(&data, 0, 0, INFO, msg);
        }
    }

    pthread_attr_destroy(&thread_attrs_detached);
    return rc;
}

/*******************************************************/
/***  initailize_threads: initializes thread context ***/
/***  initial default values.                        ***/
/*******************************************************/
void initialize_threads(struct thread_context *tctx, int num_threads)
{
    int i;
    struct thread_context *current_tctx;

    bzero(tctx, num_threads * sizeof(struct thread_context));
    for (i = 0; i < num_threads; i++) {
        current_tctx = &(tctx[i]);

        /* Initialize all variable name string. This will help in KDB debug. */
        strcpy(current_tctx->id_var, "id");
        strcpy(current_tctx->pattern_id_var, "pattern_id");
        strcpy(current_tctx->pattern_buffer_var, "pattern_buffer");
        strcpy(current_tctx->pattern_size_var, "pattern_size");
        strcpy(current_tctx->fd_var, "fd");
        strcpy(current_tctx->mode_var, "mode");
        strcpy(current_tctx->testcase_var, "testcase");
        strcpy(current_tctx->oper_var, "oper");
        strcpy(current_tctx->open_fptr_var, "open_fptr");
        strcpy(current_tctx->close_fptr_var, "close_fptr");
        strcpy(current_tctx->oper_fptr_var, "oper_fptr");
        strcpy(current_tctx->oper_count_var, "oper_count");
        strcpy(current_tctx->num_oper_var, "num_oper");
        strcpy(current_tctx->starting_block_var, "starting_block");
        strcpy(current_tctx->direction_var, "direction");
        strcpy(current_tctx->seek_breakup_prcnt_var, "seek_breakup_prcnt");
        strcpy(current_tctx->transfer_sz_var, "transfer_sz");
        strcpy(current_tctx->min_blkno_var, "min_blkno");
        strcpy(current_tctx->max_blkno_var, "max_blkno");
        strcpy(current_tctx->dlen_var, "dlen");
        strcpy(current_tctx->num_blks_var, "num_blks");
        strcpy(current_tctx->blk_hop_var, "blk_hop");
        strcpy(current_tctx->saved_dlen_var, "saved_dlen");
        strcpy(current_tctx->num_writes_var, "num_writes");
        strcpy(current_tctx->num_reads_var, "num_reads");
        strcpy(current_tctx->num_discards_var, "num_discards");
        strcpy(current_tctx->num_writes_remaining_var, "num_writes_remaining");
        strcpy(current_tctx->compare_enabled_var, "compare_enabled");
        strcpy(current_tctx->first_blk_var, "first_blk");
        strcpy(current_tctx->cur_blkno_var, "cur_blkno");
        strcpy(current_tctx->align_var, "align");
        strcpy(current_tctx->lba_align_var, "lba_align");
        strcpy(current_tctx->hotness_var, "hotness");
        strcpy(current_tctx->offset_var, "offset");
        strcpy(current_tctx->num_mallocs_var, "num_mallocs");
        strcpy(current_tctx->num_cache_threads_var, "num_cache_threads");
        strcpy(current_tctx->seed_var, "seed");
        strcpy(current_tctx->data_seed_var, "data_seed");
        strcpy(current_tctx->lba_seed_var, "lba_seed");
        strcpy(current_tctx->parent_seed_var, "parent_seed");
        strcpy(current_tctx->parent_data_seed_var, "parent_data_seed");
        strcpy(current_tctx->parent_lba_seed_var, "parent_lba_seed");
        strcpy(current_tctx->fencepost_index_var, "fencepost_index");
        strcpy(current_tctx->BWRC_zone_index_var, "BWRC_zone_index");
        strcpy(current_tctx->seg_table_index_var, "seg_table_index");
        strcpy(current_tctx->run_reread_var, "run_reread");
        strcpy(current_tctx->rule_option_var, "rule_option");
        strcpy(current_tctx->do_partial_var, "do_partial");
        strcpy(current_tctx->wptr_var, "wptr");
        strcpy(current_tctx->rptr_var, "rptr");
        strcpy(current_tctx->strt_wptr_var, "strt_wptr");
        strcpy(current_tctx->strt_rptr_var, "strt_rptr");
        strcpy(current_tctx->cur_wbuf_var, "cur_wbuf");
        strcpy(current_tctx->cur_rbuf_var, "cur_rbuf");
        strcpy(current_tctx->reread_buf_var, "reread_buf");
        strcpy(current_tctx->mis_detail_var, "mis_detail");
        strcpy(current_tctx->mis_log_buf_var, "mis_log_buf");
        strcpy(current_tctx->and_mask_var, "and_mask");
        strcpy(current_tctx->or_mask_var, "or_mask");
        strcpy(current_tctx->rand_index_var, "rand_index");
        strcpy(current_tctx->begin_dword_var, "begin_dword");
        strcpy(current_tctx->trailing_dword_var, "trailing_dword");
		strcpy(current_tctx->aio_req_queue_var, "aio_req_queue");
		strcpy(current_tctx->num_async_io_var, "num_async_io");
		strcpy(current_tctx->cur_async_io_var, "cur_async_io");
		strcpy(current_tctx->force_op_var, "force_op");
		strcpy(current_tctx->flag_var, "flag");
#ifdef __CAPI_FLASH__
		strcpy(current_tctx->open_flag_var, "open_flag");
		current_tctx->open_flag = UNDEFINED;
#endif


        /* Initialize some variable values */
        current_tctx->fd = UNDEFINED;
        current_tctx->fencepost_index = UNDEFINED;
        current_tctx->transfer_sz.min_len = UNDEFINED;
        current_tctx->BWRC_zone_index = UNDEFINED;

    }
}

/*********************************************************************/
/* execute_validation_test - executes the testcase if testcase type  */
/* is VALIDATION. It takes thread context as a input and excute      */
/* the test based on various parameters pouplated in thread context. */
/*********************************************************************/
int execute_validation_test (struct htx_data *htx_ds, struct thread_context *tctx)
{

    char tmp[64], msg[1024];
    int rc = 0, malloc_count = 0;
    int nwrites_left = 0, nreads_left = 0;
    unsigned long long oper, oper_loop, cur_num_oper;
    unsigned int opertn = 1, hot = 1, buf_alignment;
    int err_no, i = 0, j;
    volatile unsigned short write_stamping = (((hot & 0x7FF) << 5 ) | (i & 0x1F));
    unsigned short open_retry_count, offset = 0;

    /*******************************************************/
    /******     Open the disk to be exercised       ********/
    /*******************************************************/
    if (tctx->testcase == PASSTHROUGH) {
        strcpy(tmp, "/dev/");
        strcat(tmp, dev_info.disk_parent);
        open_retry_count = 1;
    } else {
        strcpy(tmp, htx_ds->sdev_id);
        open_retry_count = DEFAULT_RETRY_COUNT;
    }
#ifdef __HTX_LINUX__
    tctx->flag = O_RDWR | O_DIRECT;
#else
    tctx->flag = O_RDWR;
#endif
    for (j=0; j < open_retry_count; j++) {
        tctx->fd = (*tctx->open_func)(htx_ds, tmp, tctx);
        if (tctx->fd == -1) {
            err_no = errno;
            if (err_no == EINVAL && j < (open_retry_count - 1)) {
                sleep(30);
            } else {
                exit(1);
            }
        } else {
            break;
        }
    }

    /************************************************/
    /* Get buffer alignement                        */
    /************************************************/
    buf_alignment = get_buf_alignment(tctx);

    /*******i******************************************************/
    /* initialize seed  for random number generator               */
    /**************************************************************/
    set_seed(htx_ds, tctx);

    /********************************/
    /*  Fill pattern detail         */
    /********************************/
    rc = fill_pattern_details(htx_ds, tctx);
    if (rc == -1) {
    	 goto cleanup_pattern_buffer ;
    }

    /*******************************************************************/
    /*  For CACHE testcase, create threads equal to num_cache_threads  */
    /*  which will do cache operations.                                */
    /*******************************************************************/
    if (tctx->testcase == CACHE) {
	    create_cache_threads(htx_ds, tctx);
	}

	/*******************************************************************/
    /*  In one testcase, we can have both RANDOM as well as SEQ access */
    /*  So, we will be running 2 different loops, one for SEQ accesses */
    /*  and another one for RANDOM accesses                            */
    /*******************************************************************/

    /***********************************************/
    /****   Below code segment for SEQ access   ****/
    /***********************************************/

    if (tctx->num_oper[SEQ] != 0) { /* Means SEQ oper is defined */
        /* Set current seek stype to SEQ. Helpful in debugging at any point to
         * know whether SEQ/RANDOM oper were going on.
         */
        tctx->cur_seek_type = SEQ;
        oper_loop = 0;
        cur_num_oper = tctx->num_oper[SEQ];
        init_blkno(htx_ds, tctx);

        /***********************************************************/
        /* BWRC stanza will do only SEQ opers. We will keep        */
        /* separate code blocks for BWRC and Non-BWRC testcases.   */
        /***********************************************************/
        if (strcasecmp(tctx->oper, "BWRC") == 0) {
            /* initialize fencepost status variables for the current BWRC thread.
             * Take lock and then update.
             */
            rc = pthread_mutex_lock(&fencepost_mutex);
            if (rc) {
                sprintf(msg, "mutex lock failed while initializing fencepost, rc = %d\n", rc);
                user_msg(htx_ds, rc, 0, HARD, msg);
                goto cleanup_pattern_buffer ;
            }
            lba_fencepost[tctx->fencepost_index].status = 'R';
            rc = pthread_mutex_unlock(&fencepost_mutex);
            if (rc) {
                sprintf(msg, "mutex unlock failed while initializing fencepost, rc = %d\n", rc);
                user_msg(htx_ds, rc, 0, HARD, msg);
               	goto cleanup_pattern_buffer;
            }

            while (cur_num_oper == -1 || oper_loop < cur_num_oper) {
                if (exit_flag == 'Y' || int_signal_flag == 'Y' || signal_flag == 'Y') {
                	goto cleanup_dma_buffers;
                }

                do_boundary_check(htx_ds, tctx, oper_loop);

                /******************************************************************/
                /* malloc a new buffer each time, this allows to post new buffer  */
                /* each time for read/write, allows us to get new DMA mapping,    */
                /* adding to DMA stress.                                          */
                /******************************************************************/
                rc = allocate_buffers(htx_ds, tctx, buf_alignment, malloc_count);
                if (rc != 0) {
                	goto cleanup_dma_buffers;
                }
                tctx->wbuf = tctx->strt_wptr[malloc_count] + offset;
                tctx->rbuf = tctx->strt_rptr[malloc_count] + offset;

                bldbuf(htx_ds, tctx, write_stamping);

                if (htx_ds->run_type[0] == 'O' || dev_info.debug_flag == 1) {
                    DPRINT("%s:%d - thread_id=%s, wbuf=%#llx, rbuf=%#llx, dlen=%#llx, num_blks = %#x, blkno: %#llx, bytpsec = %#x, mallocount =  hx%#x, hot=%#x, hotness=%#x \n",
                            __FUNCTION__, __LINE__, tctx->id, (unsigned long long)tctx->wbuf, (unsigned long long)tctx->rbuf, tctx->dlen, tctx->num_blks, tctx->blkno[0],
                            dev_info.blksize, malloc_count, hot, tctx->hotness);
                }

                seg_lock(htx_ds, tctx, tctx->blkno[0], (tctx->blkno[0] + tctx->num_blks - 1));
                for(oper = 0; oper < tctx->oper_count; oper++) {
                    rc = (*(tctx->operations[oper]))(htx_ds, tctx, oper_loop);
                    if (rc) {
                        break;
                    }
                }
                seg_unlock(htx_ds, tctx, tctx->blkno[0], (tctx->blkno[0] + tctx->num_blks - 1));
                if (rc) {
                	goto cleanup_dma_buffers;
                }
#if 0
                if ((oper_loop % 50) == 0) {
                    hxfupdate(UPDATE, htx_ds); /* update htx statistics */
                }
#endif
                set_blkno(htx_ds, tctx);

                offset = (offset + tctx->offset) % 64;
                if (malloc_count >= tctx->num_mallocs - 1) {
                    free_buffers(&malloc_count, tctx);
                } else {
                    malloc_count++;
                }
                oper_loop++;
            } /* End for num_oper loop */
        } else { /* else for BWRC oper check */
            if (tctx->num_discards != 0) {
                opertn = tctx->num_discards;
            } else if (tctx->num_reads == 0) {
                opertn = tctx->num_writes;
            } else if (tctx->num_writes == 0) {
                opertn = tctx->num_reads;
            } else {
                opertn = (tctx->num_reads <= tctx->num_writes) ? tctx->num_reads : tctx->num_writes;
                nwrites_left = tctx->num_writes - opertn;
                nreads_left = tctx->num_reads - opertn;
            }

            /****************************************************************/
            /* If there are BWRC threads defined and operation for current  */
            /* thread is not BWRC, We need to make sure that lba_fencepost  */
            /* of BWRC zone in which current thread lies has reached to a   */
            /* point where current thread can do initial operation,         */
            /* otherwise, hold the thread from running                      */
            /****************************************************************/
            if ((total_BWRC_threads != 0) && (tctx->num_writes == 0) && (tctx->compare_enabled == 'y')) {
                wait_for_fencepost_catchup(tctx);
            }

            while (cur_num_oper == -1 || oper_loop < cur_num_oper) {
                if (exit_flag == 'Y' || int_signal_flag == 'Y' || signal_flag == 'Y') {
                	goto cleanup_dma_buffers;
                }
                do_boundary_check(htx_ds, tctx, oper_loop);

                /*******************************************************/
                /* do fencepost check only if BWRC threads are running */
                /* and compare operation is defined OR Write operation */
                /* is not defined.                                     */
                /*******************************************************/
                if ((total_BWRC_threads != 0) && (strcasecmp(tctx->oper, "wrc") != 0)) {
					do_fencepost_check(htx_ds, tctx);
                }

                /*******************************************************/
                /* malloc a new buffer each time, this allows to post  */
                /* new buffer each time for read/write, allows us to   */
                /* get new DMA mapping, adding to DMA stress.          */
                /*******************************************************/
                rc = allocate_buffers(htx_ds, tctx, buf_alignment, malloc_count);
                if (rc != 0) {
                	goto cleanup_dma_buffers;
                }
                tctx->wbuf = tctx->strt_wptr[malloc_count] + offset;
                tctx->rbuf = tctx->strt_rptr[malloc_count] + offset;

                bldbuf(htx_ds, tctx, write_stamping);

                for (hot = 1; hot <= tctx->hotness; hot++) {
                    tctx->num_writes_remaining = tctx->num_writes;
                    for (i = 0; i < opertn; i++) {
                        write_stamping = (((hot & 0x7FF) << 5 ) | (i & 0x1F));
                        /*************************************************************/
                        /* Need to build buffer everytime if hotness is 1 except for */
                        /* the first iteration in loop. Otherwise, just update time  */
                        /* and write stamping as blkno will be same.                 */
                        /*************************************************************/
                        if (tctx->hotness == 1) {
                            if (i > 0) {
                                set_blkno(htx_ds, tctx);
                                do_boundary_check(htx_ds, tctx, oper_loop);
                                if ((total_BWRC_threads != 0) && (strcasecmp(tctx->oper, "wrc") != 0)) {
									do_fencepost_check(htx_ds, tctx);
                                }
                                bldbuf(htx_ds, tctx, write_stamping);
                            }
                        } else if (tctx->pattern_id[0] == '#') {
                            /* Update write stamping/ timestamp and cksum  if hotness is greater than 1 */
                            update_header(tctx, write_stamping);
                        }
                        if (htx_ds->run_type[0] == 'O' || dev_info.debug_flag == 1) {
                            DPRINT("%s: wbuf=%#llx, rbuf=%#llx, dlen=%#llx, num_blks = %#x, blkno: %#llx, bytpsec = %#x, mallocount = %#x, hot=%#x, hotness=%#x \n",
                                    tctx->id, (unsigned long long)tctx->wbuf, (unsigned long long)tctx->rbuf, tctx->dlen, tctx->num_blks, tctx->blkno[0], dev_info.blksize, malloc_count, hot, tctx->hotness);
                        }
                        seg_lock(htx_ds, tctx, tctx->blkno[0], (tctx->blkno[0] + tctx->num_blks - 1));
                        for (oper = 0; oper < tctx->oper_count; oper++) {
                            rc = (*(tctx->operations[oper])) (htx_ds, tctx, oper_loop);
                            if (rc) {
                                break;
                            }
                        }
                        seg_unlock(htx_ds, tctx, tctx->blkno[0], (tctx->blkno[0] + tctx->num_blks - 1));
                        if (rc) {
                        	goto cleanup_dma_buffers;
                        }
                        tctx->num_writes_remaining--;
                    } /* End i < opertn loop */
                    for (i = 0; i < nwrites_left; i++) {
                        write_stamping = (((hot & 0x7FF) << 5 ) | ((opertn + i) & 0x1F));
                        if (tctx->hotness == 1) {
                            set_blkno(htx_ds, tctx);
                            do_boundary_check(htx_ds, tctx, oper_loop);
                            bldbuf(htx_ds, tctx, write_stamping);
                        } else if (tctx->pattern_id[0] == '#') {
                            update_header(tctx, write_stamping);
                        }
                        seg_lock(htx_ds, tctx, tctx->blkno[0], (tctx->blkno[0] + tctx->num_blks - 1));
                        rc = (*(tctx->operations[0])) (htx_ds, tctx, oper_loop);
                        if (rc) {
                            break;
                        }
                        seg_unlock(htx_ds, tctx, tctx->blkno[0], (tctx->blkno[0] + tctx->num_blks - 1));
                        if (rc) {
							goto cleanup_dma_buffers;
                        }
                        tctx->num_writes_remaining--;
                    } /* End i < nwrites_left loop */

                    for (i = 0; i < nreads_left; i++) {
                        seg_lock(htx_ds, tctx, tctx->blkno[0], (tctx->blkno[0] + tctx->num_blks - 1));
                        for (oper = 1; oper < tctx->oper_count; oper++) {
                            rc = (*(tctx->operations[oper])) (htx_ds, tctx, oper_loop);
                            if (rc) {
                                break;
                            }
                        }
                        seg_unlock(htx_ds, tctx, tctx->blkno[0], (tctx->blkno[0] + tctx->num_blks - 1));
                        if (rc) {
                       		goto cleanup_dma_buffers;
                        }
                    }

#if 0
                    if ((hot % 50) == 0) {
                        hxfupdate(UPDATE, htx_ds); /* update htx statistics */
                    }
#endif
                } /* End hotness loop */

#if 0
                if ((oper_loop % 20) == 0) {
                    hxfupdate(UPDATE, htx_ds); /* update htx statistics */
                }
#endif
                if (tctx->transfer_sz.increment != 0) {
                    random_dlen(tctx);
                }
                set_blkno(htx_ds, tctx);

                offset = (offset + tctx->offset) % 64;
                if (malloc_count >= tctx->num_mallocs - 1) {
                    free_buffers(&malloc_count, tctx);
                } else {
                    malloc_count++;
                }
                oper_loop++;
            } /* End for num_oper loop */
            if (tctx->testcase == CACHE) {
			    wait_for_cache_threads_completion(htx_ds, tctx);
		    }
        } /* End if for BWRC check */
    } /* End if for SEQ access check */

    /*************************************************/
    /**** Below code section for RANDOM access   *****/
    /*************************************************/
    if (tctx->num_oper[RANDOM] != 0) { /* means RANDOM access is defined */
        /* Set current seek stype to SEQ. Helpful in debugging at any point to
         * know whether SEQ/RANDOM oper were going on.
         */
        tctx->cur_seek_type = RANDOM;
        if (tctx->transfer_sz.increment == -1 && tctx->dlen != tctx->transfer_sz.min_len) {
            tctx->dlen = tctx->transfer_sz.min_len;
            tctx->num_blks = tctx->dlen / dev_info.blksize;
        }
        oper_loop = 0;
        cur_num_oper = tctx->num_oper[RANDOM];

        if (tctx->num_discards != 0) {
            opertn = tctx->num_discards;
        } else if (tctx->num_reads == 0) {
            opertn = tctx->num_writes;
        } else if (tctx->num_writes == 0) {
            opertn = tctx->num_reads;
        } else {
            opertn = (tctx->num_reads <= tctx->num_writes) ? tctx->num_reads : tctx->num_writes;
            nwrites_left = tctx->num_writes - opertn;
            nreads_left = tctx->num_reads - opertn;
        }

        while (oper_loop < cur_num_oper || cur_num_oper == -1) {
            if (exit_flag == 'Y' || int_signal_flag == 'Y' || signal_flag == 'Y') {
            	goto cleanup_dma_buffers;
            }

            /********************************************************/
            /*  malloc a new buffer each time, this allows to post  */
            /*  new buffer each time for read/write, allows us to   */
            /*  get new DMA mapping, adding to DMA stress.          */
            /********************************************************/
            rc = allocate_buffers(htx_ds, tctx, buf_alignment, malloc_count);
            if (rc) {
            	goto cleanup_dma_buffers;
            }
            tctx->wbuf = tctx->strt_wptr[malloc_count] + offset;
            tctx->rbuf = tctx->strt_rptr[malloc_count] + offset;

            if (tctx->rule_option & RESTORE_SEEDS_FLAG && tctx->transfer_sz.increment == 0) {
                random_blkno(htx_ds, tctx, saved_data_len);
            } else {
                random_blkno(htx_ds, tctx, tctx->dlen);
            }
            bldbuf(htx_ds, tctx, write_stamping);

            for (hot = 1; hot <= tctx->hotness; hot++) {
                tctx->num_writes_remaining = tctx->num_writes;
                for (i =0; i < opertn; i++) {
                    write_stamping = (hot & 0x7FF) << 5 | (i & 0x1F);
                    /***************************************************/
                    /* Need to set blkno and build buffer if hotness   */
                    /* is 1 except for the first time in the loop.     */
                    /* Otherwise, just update time and write stamping  */
                    /* as blkno. will be same.                         */
                    /***************************************************/
                    if (tctx->hotness == 1) {
                        if (i > 0) {
                            if (tctx->rule_option & RESTORE_SEEDS_FLAG && tctx->transfer_sz.increment == 0) {
                                random_blkno(htx_ds, tctx, saved_data_len);
                            } else {
                                random_blkno(htx_ds, tctx, tctx->dlen);
                            }
                            bldbuf(htx_ds, tctx, write_stamping);
                        }
                    } else if (tctx->pattern_id[0] == '#') {
                        update_header(tctx, write_stamping);
                    }
                    if (htx_ds->run_type[0] == 'O' || dev_info.debug_flag == 1) {
                        DPRINT("%s: wbuf=%#llx, rbuf=%#llx, dlen=%#llx, num_blks = %#x, blkno: %#llx, bytpsec = %#x, mallocount = %#x, hot=%#x, hotness=%#x \n",
                                tctx->id, (unsigned long long)tctx->wbuf, (unsigned long long)tctx->rbuf, tctx->dlen, tctx->num_blks, tctx->blkno[0], dev_info.blksize, malloc_count, hot, tctx->hotness);
                    }

                    seg_lock(htx_ds, tctx, tctx->blkno[0], (tctx->blkno[0] + tctx->num_blks - 1));
                    for (j = 0; j < tctx->oper_count; j++) {
                        rc = (*(tctx->operations[j])) (htx_ds, tctx, oper_loop);
                        if (rc) {
                            break;
                        }
                    }
                    seg_unlock(htx_ds, tctx, tctx->blkno[0], (tctx->blkno[0] + tctx->num_blks - 1));
                    if (rc) {
                    	goto cleanup_dma_buffers;
                    }
                    tctx->num_writes_remaining--;
                } /* end for i < opertn loop */

                for (i = 0; i < nwrites_left; i++) {
                    if (tctx->hotness == 1) {
                        if (tctx->rule_option & RESTORE_SEEDS_FLAG && tctx->transfer_sz.increment == 0) {
                            random_blkno(htx_ds, tctx, saved_data_len);
                        } else {
                            random_blkno(htx_ds, tctx, tctx->dlen);
                        }
                        bldbuf(htx_ds, tctx, write_stamping);
                    } else if (tctx->pattern_id[0] == '#') {
                        write_stamping = (((hot & 0x7FF) << 5 ) | ((opertn + i) & 0x1F));
                        update_header(tctx, write_stamping);
                    }
                    seg_lock(htx_ds, tctx, tctx->blkno[0], (tctx->blkno[0] + tctx->num_blks - 1));
                    rc = (*(tctx->operations[j])) (htx_ds, tctx, oper_loop);
                    if (rc) {
                        break;
                    }
                    seg_unlock(htx_ds, tctx, tctx->blkno[0], (tctx->blkno[0] + tctx->num_blks - 1));
                    if (rc) {
                    	goto cleanup_dma_buffers;
                    }
                    tctx->num_writes_remaining--;
                } /* end for i < nwrites_left loop */

                for (i = 0; i < nreads_left; i++) {
                    seg_lock(htx_ds, tctx, tctx->blkno[0], (tctx->blkno[0] + tctx->num_blks - 1));
                    for (j = 1; j < tctx->oper_count; j++) {
                        rc = (*(tctx->operations[j])) (htx_ds, tctx, oper_loop);
                        if (rc) {
                            break;
                        }
                    }
                    seg_unlock(htx_ds, tctx, tctx->blkno[0], (tctx->blkno[0] + tctx->num_blks - 1));
                    if (rc) {
						goto cleanup_dma_buffers;
                    }
                } /* End for i < nreads_left loop */

#if 0
                if ((hot % 50) == 0) {
                    hxfupdate(UPDATE, htx_ds); /* update htx statistics */
                }
#endif
            } /* End for hotness loop */
#if 0
            if ((oper_loop % 20) == 0) {
                hxfupdate(UPDATE, htx_ds); /* update htx statistics */
            }
#endif
            /*****************************************************/
            /* Get the length if size is not fixed. To maintain  */
            /* read/write ratio, length will always be same in   */
            /* the hotness loop, so kept it outside the loop.    */
            /*****************************************************/
            if (tctx->transfer_sz.increment != 0) {
                random_dlen(tctx);
            }

            offset = (offset + tctx->offset) % 64;
            if (malloc_count >= tctx->num_mallocs - 1) {
                free_buffers(&malloc_count, tctx);
            } else {
                malloc_count++;
            }
            oper_loop++;
        }
    } /* End if condition check for RANDOM access */

cleanup_dma_buffers :
    /* Free read/write buffers */
    free_buffers(&malloc_count, tctx);

cleanup_pattern_buffer :
    /* Free pattern buffer */
    free_pattern_buffers(tctx);

	if (tctx->testcase == CACHE) {
    	wait_for_cache_threads_completion(htx_ds, tctx);
    }

    /* close the disk */
	tctx->force_op = rc;
    rc = (*tctx->close_func)(htx_ds, tctx);
    if (rc) {
        sprintf(msg, "%s:%s: CLOSE call failed, rc = %d !, errno = %d \n", __FUNCTION__, tctx->id, rc, errno);
        user_msg(htx_ds, errno, 0, HARD, msg);
        exit(1);
    }
    return(rc);
}

int execute_performance_test (struct htx_data *htx_ds, struct thread_context *tctx)
{

    char tmp[64], msg[512];
    unsigned long long oper_loop=0, oper;
    int i, malloc_count=0, rc = 0;
    unsigned int buf_alignment;
    volatile unsigned short write_stamping = 1;

    /*******************************************************/
    /******     Open the disk to be exercised       ********/
    /*******************************************************/
    strcpy(tmp, htx_ds->sdev_id);
#ifdef __HTX_LINUX__
    tctx->flag = O_RDWR | O_DIRECT;
#else
    tctx->flag = O_RDWR;
#endif
	tctx->fd = (*tctx->open_func)(htx_ds, tmp, tctx);
    if (tctx->fd == -1) {
        sprintf(msg, "In performace test - open for disk failed");
        user_msg(htx_ds, errno, 0, HARD, msg);
        exit(1);
    }

    /**************************************************************/
    /* initialize seed  for random number generator               */
    /**************************************************************/
    set_seed(htx_ds, tctx);

    /************************************************/
    /* get buffer alignement                        */
    /************************************************/
    buf_alignment = get_buf_alignment(tctx);

    /********************************/
    /*  Fill pattern detail         */
    /********************************/
    rc = fill_pattern_details(htx_ds, tctx);
    if (rc == -1) {
    	goto cleanup_pattern_buffer ;
    }
    /***************************************************************
     * For ASYNC we need to keep track IOs inflight, user would
     * specify num_async_io , create data structure to keep
     * track of all IOs
     ***************************************************************/
    if(tctx->testcase == ASYNC) {

        if(tctx->num_async_io == 0) {
            /* Assume QD, should not de done here, If we came here with 0 then hardcode  */
            tctx->num_async_io = 32;
        }
        tctx->aio_req_queue =(struct aio_ops*) malloc (sizeof(struct aio_ops) * tctx->num_async_io);
        if(tctx->aio_req_queue == NULL) {
            sprintf(msg, "In performace test - malloc for aio_req_queue failed, errno = %d \n", errno);
            user_msg(htx_ds, errno, 0, HARD, msg);
            rc = -1;
            goto cleanup_pattern_buffer ;
        }
        memset(tctx->aio_req_queue, 0, (sizeof(struct aio_ops) * tctx->num_async_io));
        tctx->cur_async_io = 0;
    } else {
        tctx->aio_req_queue = NULL;
        tctx->num_async_io = 1;
		tctx->cur_async_io = 1;
    }



    /********************/
    /* allocate buffers */
    /********************/
    malloc_count = allocate_buffers(htx_ds, tctx, buf_alignment, tctx->num_mallocs);
    if (malloc_count) {
        tctx->num_mallocs = malloc_count;
        goto cleanup_dma_buffers;
    }

    /********************************/
    /*  Build buffer once only      */
    /********************************/
    for (i = 0; i < tctx->num_mallocs; i++) {
        tctx->wbuf = tctx->strt_wptr[i];
        bldbuf(htx_ds, tctx, write_stamping);
    }

    if (tctx->seek_breakup_prcnt == 0) { /* means SEQ access */
        init_blkno(htx_ds, tctx);
        while (oper_loop < tctx->num_oper[SEQ] || tctx->num_oper[SEQ] == -1) {
            if (exit_flag == 'Y' || int_signal_flag == 'Y' || signal_flag == 'Y') {
				goto cleanup_dma_buffers ;
            }

            tctx->wbuf = tctx->strt_wptr[malloc_count];
            tctx->rbuf = tctx->strt_rptr[malloc_count];

            do_boundary_check(htx_ds, tctx, oper_loop);

            if (htx_ds->run_type[0] == 'O' || dev_info.debug_flag == 1) {
                DPRINT("%s: wbuf=%#llx, rbuf=%#llx, dlen=%#llx, num_blks = %#x, blkno: %#llx, bytpsec = %#x, mallocount = %#x \n",
                        tctx->id, (unsigned long long)tctx->wbuf, (unsigned long long)tctx->rbuf, tctx->dlen, tctx->num_blks, tctx->blkno[0], dev_info.blksize, malloc_count);
            }

            /* do operation */
            for(oper = 0; oper < tctx->oper_count; oper++) {
                rc = (*(tctx->operations[oper])) (htx_ds, tctx, oper_loop);
                if (rc) {
                    break;
                }
            }
            if (rc) {
				 goto cleanup_dma_buffers ;
            }
#if 0
            if (oper_loop % 50 == 0) {
                hxfupdate(UPDATE, htx_ds); /* update htx statistics */
            }
#endif
            set_blkno(htx_ds, tctx);
            malloc_count = (malloc_count + 1) % tctx->num_mallocs;
            oper_loop++;
        }
    } else { /* else for SEQ access check */
        oper_loop = 0;
        while (oper_loop < tctx->num_oper[RANDOM] || tctx->num_oper[RANDOM] == -1) {
            if (exit_flag == 'Y' || int_signal_flag == 'Y' || signal_flag == 'Y') {
				goto cleanup_dma_buffers ;
            }
            tctx->wbuf = tctx->strt_wptr[malloc_count];
            tctx->rbuf = tctx->strt_rptr[malloc_count];
            random_blkno(htx_ds, tctx, tctx->dlen);

            if (htx_ds->run_type[0] == 'O' || dev_info.debug_flag == 1) {
                DPRINT("%s: wbuf=%#llx, rbuf=%#llx, dlen=%#llx, num_blks = %#x, blkno: %#llx, bytpsec = %#x, mallocount = %#x \n",
                        tctx->id, (unsigned long long)tctx->wbuf, (unsigned long long)tctx->rbuf, tctx->dlen, tctx->num_blks, tctx->blkno[0], dev_info.blksize, malloc_count);
            }

            /* do operation */
            for (oper = 0; oper < tctx->oper_count; oper++) {
                rc = (*(tctx->operations[oper])) (htx_ds, tctx, oper_loop);
                if (rc) {
                    break;
                }
            }
            if (rc) {
            	goto cleanup_dma_buffers ;
            }
#if 0
            if (oper_loop % 100 == 0) {
                hxfupdate(UPDATE, htx_ds); /* update htx statistics */
            }
#endif
            malloc_count = (malloc_count + 1) % tctx->num_mallocs;
            oper_loop++;
        }
    } /* End if check for SEQ/RQNDOM access */
cleanup_dma_buffers :
#ifdef __CAPI_FLASH__
    if(tctx->cur_async_io && tctx->testcase == ASYNC) {
        /* Exiting .. cleanup all the IOs issued */
        while(tctx->cur_async_io)  {
            rc = cflsh_aresult_operation(htx_ds, tctx, oper_loop);
        }
    }
#endif
    /* Free read/write bufs */
    free_buffers(&(tctx->num_mallocs), tctx);

cleanup_pattern_buffer :
    /* Free pattern buffer */
    free_pattern_buffers(tctx);
    if (tctx->testcase == ASYNC) {
        free(tctx->aio_req_queue);
    }

    /* close the disk */
	tctx->force_op = rc;
    rc = (*tctx->close_func)(htx_ds, tctx);
    if (rc) {
        sprintf(msg, "%s : CLOSE call failed!, rc = %d, errno = %d", __FUNCTION__, rc, errno);
        user_msg(htx_ds, errno, 0, HARD, msg);
        exit(1);
    }
    return rc;

}

void * execute_thread_context (void *th_context)
{
    struct thread_context *tctx = (struct thread_context *)th_context;
    struct htx_data htx_ds;
    int rc;
    char msg[512];

    rc = pthread_mutex_lock(&thread_create_mutex);
    if (rc) {
        sprintf(msg, "First mutex lock failed in function execute_thread_context, rc = %d\n", rc);
        user_msg(&data, rc, 0, HARD, msg);
        exit(rc);
    }

    /* Any individual thread related updation, we can do here. i.e. between mutex lock
     * and broadcast.
     */
    /* print_thread_context(tctx); */
    memcpy(&htx_ds, &data, sizeof(struct htx_data));

    /****************************************************************************/
    /***** Braodcast for main thread to inform that thread has been created     */
    /****************************************************************************/
    rc = pthread_cond_broadcast(&create_thread_cond_var);
    if (rc) {
        sprintf(msg, "Cond broadcast failed for create_thread_cond_var in function execute_thread_context, rc = %d\n", rc);
        user_msg(&htx_ds, rc, 0, HARD, msg);
        exit(rc);
    }

    /************************************************************/
    /*****      Wait for all threads to get created         *****/
    /************************************************************/
    rc = pthread_cond_wait(&do_oper_cond_var, &thread_create_mutex);
    if (rc) {
        sprintf(msg, "Cond wait failed in function execute_thread_context, rc = %d\n", rc);
        user_msg(&htx_ds, rc, 0, HARD, msg);
        exit(rc);
    }

    rc = pthread_mutex_unlock(&thread_create_mutex);
    if (rc) {
        sprintf(msg, "First mutex unlock failed in function execute_thread_context, rc = %d\n", rc);
        user_msg(&htx_ds, rc, 0, HARD, msg);
        exit(rc);
    }

    if (tctx->mode == PERFORMANCE) {
        rc = execute_performance_test(&htx_ds, tctx);
    } else {
        rc = execute_validation_test(&htx_ds, tctx);
    }

    /* Update lba_fencepost.status flag if oper is BWRC
     * Need to take sgement_mutex lock for it.
     */
    if (strcasecmp(tctx->oper, "BWRC") == 0) {
        rc = pthread_mutex_lock(&fencepost_mutex);
        if (rc) {
            sprintf(msg, "mutex lock failed for fencepost_mutex in function execute_thread_context, rc = %d\n", rc);
            user_msg(&htx_ds, rc, 0, HARD, msg);
            exit(rc);
        }

        lba_fencepost[tctx->fencepost_index].status = 'F';

        rc = pthread_mutex_unlock(&fencepost_mutex);
        if (rc) {
            sprintf(msg, "mutex unlock failed for fencepost_mutex in function execute_thread_context, rc = %d\n", rc);
            user_msg(&htx_ds, rc, 0, HARD, msg);
            exit(rc);
        }
    }

    /* Need to take thread_create_mutex lock and then update variables */
    rc = pthread_mutex_lock(&thread_create_mutex);
    if (rc) {
        sprintf(msg, "2nd mutex lock failed in function execute_thread_context, rc = %d\n", rc);
        user_msg(&htx_ds, rc, 0, HARD, msg);
        exit(rc);
    }

    if (strcasecmp(tctx->oper, "BWRC") == 0) {
        BWRC_threads_running--;
    } else {
        non_BWRC_threads_running--;
        if (non_BWRC_threads_running == 0) {
            rc = pthread_cond_broadcast(&threads_finished_cond_var);
            if (rc) {
                sprintf(msg, "Cond broadcast failed threads_finished_cond_var for in function execute_thread_context, rc = %d\n", rc);
                user_msg(&htx_ds, rc, 0, HARD, msg);
                exit(rc);
            }
        }
    }

    rc = pthread_mutex_unlock(&thread_create_mutex);
    if (rc) {
        sprintf(msg, "2nd mutex unlock failed in function execute_thread_context, rc = %d\n", rc);
        user_msg(&htx_ds, rc, 0, HARD, msg);
        exit(rc);
    }
    pthread_exit(NULL);
}

/**********************************************/
/*    Thread function to update the stats     */
/**********************************************/
void stats_update_thread()
{
    struct htx_data htx_d;
    int time_spent, rc = 0;
    char msg[128];

    memcpy(&htx_d, &data, sizeof(struct htx_data));

    user_msg(&htx_d, rc, 0, INFO, "created stats thread\n");
    while (1) {
        time_spent = 0;
        while ((time_spent++) < STATS_UPDATE_INTERVAL) {
            sleep(1);
            if (exit_flag == 'Y' || exit_flag == 'y') {
                break;
            }
        }
        rc = pthread_mutex_lock (&stats_mutex);
        if (rc) {
            sprintf(msg, "Mutex lock failed for stats thread, rc = %d\n", rc);
            user_msg(&htx_d, rc, 0, HARD, msg);
            exit(1);
        }
        htx_d.test_id = data.test_id;
        htx_d.good_reads = global_htx_d.good_reads;
        htx_d.good_writes = global_htx_d.good_writes;
        htx_d.bytes_read = global_htx_d.bytes_read;
        htx_d.bytes_writ = global_htx_d.bytes_writ;

        htx_d.bad_writes = global_htx_d.bad_writes;
        htx_d.bad_reads = global_htx_d.bad_reads;

        global_htx_d.good_reads = 0;
        global_htx_d.good_writes = 0;
        global_htx_d.bytes_read = 0;
        global_htx_d.bytes_writ = 0;
        global_htx_d.bad_writes = 0;
        global_htx_d.bad_reads = 0;

        rc = pthread_mutex_unlock (&stats_mutex);
        if (rc) {
            sprintf(msg, "Mutex unlock failed for stats thread, rc = %d\n", rc);
            user_msg(&htx_d, rc, 0, HARD, msg);
            exit(1);
        }

        if (htx_d.good_reads > 0 || htx_d.good_writes  > 0 || htx_d.bytes_read > 0 || htx_d.bytes_writ > 0) {
            HTX_STATS_UPDATE(UPDATE, &htx_d);
        }
        if (exit_flag == 'Y' || exit_flag == 'y') {
            pthread_exit((void *) 0);
        }
    }
}

/********************************************************************/
/*  Create cache threads for CACHE testcase  based upon operation   */
/*  types defined in oper variable                                  */
/********************************************************************/
int create_cache_threads(struct htx_data *htx_ds, struct thread_context *tctx)
{
    if ((strcasecmp(tctx->oper, "CARR") == 0) || (strcasecmp(tctx->oper, "CARW") == 0)) {
	    read_cache_disk(htx_ds, tctx);
	} else {
	    write_cache_disk(htx_ds, tctx);
	}
	return 0;
}

#ifdef __HTX_LINUX__
/*********************************************************************/
/**  Creates thread for doing sync_cache ioctl at regular interval  **/
/*********************************************************************/
int sync_cache_thread(struct htx_data *htx_ds)
{
    int rc = 0, sync_fd, random_sleep_time;
    long seed, tmp;
	char msg[128];
	struct drand48_data drand_buf;

    sync_fd = open(htx_ds->sdev_id, O_RDWR);
    if (sync_fd == -1) {
        sprintf(msg, "open for device %s failed in sync_cache_thread. errno is: %d\n", htx_ds->sdev_id, errno);
        user_msg(htx_ds, 0, 0, HARD, msg);
        return (-1);
    }
    /* Initialize the random no. generator structure */
    seed = (long) (time(0));
    srand48_r (seed, &drand_buf);

    while (1) {
        if (exit_flag == 'Y' || int_signal_flag == 'Y') {
            break;
	    }

	    /* This thread will wakeup at random intervals between 15 sec. - 90 sec and
	     * does sync_cache operation
	     */
	    lrand48_r (&drand_buf, &tmp);
	    random_sleep_time = MIN_SLEEP_TIME + (tmp % (MAX_SLEEP_TIME - MIN_SLEEP_TIME));
	    /* printf("Sleeping for %d sec\n", random_sleep_time); */
	    sleep(random_sleep_time);

	    rc = sync_cache_operation(htx_ds, sync_fd);
	    if (rc) {
            close(sync_fd);
            return rc;
	    }
    }
    close(sync_fd);
    return (rc);
}
#endif

/********************************************************************/
/*  Fills the data in pattern_buffer based on pattern_id defined    */
/*  for the thread. Various pattern_id types can be 0x64891acfe,    */
/*  HEX255, #003.                                                   */
/********************************************************************/
int fill_pattern_details(struct htx_data *htx_ds, struct thread_context *tctx)
{
    unsigned int len, j;
    char msg[128], path[100];
    unsigned int *rand_buffer;
    char str[8], *tmp_ptr = NULL;
    int err_no, rc = 0;

    if (strncmp(tctx->pattern_id, "0x", 2) == 0) { /* pattern defined explicitly e.g. 0x12345678 */
        len = strlen(tctx->pattern_id);
        len -= 2;
        if (len == 0) {
            sprintf(msg, "pattern can not be of zero length.");
            user_msg(htx_ds, 0, 0, HARD, msg);
            return (-1);
        }
        if (len > MAX_PATTERN_LENGTH) { /* max. allowed length is 16 bytes. 2 characters means 1 byte */
            sprintf(msg, "max. pattern size allowed is 16 bytes.\n");
            user_msg(htx_ds, 0, 0, HARD, msg);
            return (-1);
        }
        if (len & (len - 1)) {
            sprintf(msg, "pattern's size should be power of 2 bytes\n");
            user_msg(htx_ds, 0, 0, HARD, msg);
            return (-1);
        }
        tctx->pattern_size = len/2;

        tctx->pattern_buffer = (char *) malloc(len/2);
        if (tctx->pattern_buffer == NULL) {
            err_no = errno;
            sprintf(msg, "malloc failed for pattern_buffer with errno: %d\n", errno);
            user_msg(htx_ds, err_no, 0, HARD, msg);
            return (-1);
        }
        /* DPRINT("pattern buffer is: 0x%llx\n", tctx->pattern_buffer); */
        j = 0;
        while (j < len) {
            strncpy(str, &tctx->pattern_id[j], 4); /* Added 2 in pattern_id array index to omit initial 0x */
            str[4] = '\0';
            *(unsigned short *)(&tctx->pattern_buffer[2*(j/4)]) = strtoull(str, NULL, 16);
            j += 4;
        }
    } else if (tctx->pattern_id[0] == '#') {  /* Pattern beging with # e.g. #003 */
       if (tctx->pattern_id[3] == '8' ||
           tctx->pattern_id[3] == '9' ||
           tctx->pattern_id[3] == 'A') {
            tctx->and_mask = 0x1;
            tctx->or_mask = 0x8000000000000000ULL;
            tctx->rand_index = 0;
            tctx->begin_dword = 0;
            tctx->trailing_dword = ((tctx->align / sizeof(unsigned long long)) -1);

            /* Just In case if I need random numbers fill the random buffer with
             * random numbers in num_oper we offset and pick numbers from here
             */
            tctx->pattern_buffer = (char *) malloc(tctx->transfer_sz.max_len + dev_info.blksize + 128);
            tctx->pattern_size = tctx->transfer_sz.max_len + dev_info.blksize + 128;
            if (tctx->pattern_buffer == NULL) {
                err_no = errno;
                sprintf(msg, "malloc failed for pattern_buffer with errno: %d\n", errno);
                user_msg(htx_ds, err_no, 0, HARD, msg);
                return (-1);
            }

            rand_buffer = (unsigned int *) tctx->pattern_buffer;
            for (j=0; j < tctx->pattern_size/sizeof(unsigned int); j++) {
                *(unsigned int *)rand_buffer = (unsigned int)get_random_number(&(tctx->seed));
                rand_buffer++;
            }
        }
    } else if (tctx->pattern_id[0] != '#') { /* pattern taken from HTX Pattern library */
        tctx->pattern_buffer = (char *) malloc(tctx->transfer_sz.max_len + dev_info.blksize + 128);
        if (tctx->pattern_buffer == NULL) {
            err_no = errno;
            sprintf(msg, "malloc failed for pattern_buffer with errno: %d\n", errno);
            user_msg(htx_ds, err_no, 0, HARD, msg);
            return (-1);
        }
        tctx->pattern_size = tctx->transfer_sz.max_len + dev_info.blksize + 128;
        tmp_ptr = getenv("HTXPATTERNS");
        if (tmp_ptr != NULL) {
            strcpy (path, tmp_ptr);
        } else {
            sprintf(msg, "HTX env. is not set. Hence exiting.\nPlease setup the env. and re-run the test");
            user_msg(htx_ds, 0, 0, INFO, msg);
            exit(1);
	    }
        strcat (path, tctx->pattern_id);
        rc = hxfpat(path, tctx->pattern_buffer, tctx->pattern_size);
        if (rc == 1) {
            sprintf(msg, "cannot open pattern file - %s\n", path);
            user_msg(htx_ds, 0, 0, HARD, msg);
            return (-1);
        }
        if (rc == 2) {
            sprintf(msg, "cannot read pattern file - %s\n", path);
            user_msg(htx_ds, 0, 0, HARD, msg);
            return (-1);
        }
    }
    return rc;
}

/************************************************************/
/* Function to update the min_blkno for each rule if state  */
/* table is enabled. As initial few blocks will be used to  */
/* maintain the metadata.                                   */
/************************************************************/
void update_min_blkno()
{
   int i, j;

    for (i = 0; i < num_rules_defined; i++) {
        for (j = 0; j < rule_list[i].min_blkno.num_keywds; j++) {
            if ( rule_list[i].min_blkno.value[j] < ((s_tbl.size / dev_info.blksize) + 1)) {
                rule_list[i].min_blkno.value[j] = (s_tbl.size / dev_info.blksize) + 1;
            }
        }
    }
}

/************************************************************/
/*  Function to parse the operations defined for the thread */
/*  and update the corresponding function pointers in the   */
/*  thread context structure.                               */
/************************************************************/
int populate_operations(struct thread_context *tctx)
{
    int len;
    int i = 0, j, k;
    unsigned short w_found = 0, r_found = 0;
    char num[10];

    len = strlen(tctx->oper);
    tctx->oper_count = 0;
    tctx->compare_enabled = 'n';
    if (tctx->oper[i] == 'B') { /* means oper is BWRC, skip char 'B' */
        i++;
    }
    if (strncasecmp(tctx->oper, "CA", 2) == 0 ) { /* means cache operations */
        if ((strcasecmp(tctx->oper, "CARW") == 0) || (strcasecmp(tctx->oper, "CARR") == 0)) {
            tctx->operations[tctx->oper_count++] = oper_list[SYNC][W];
            tctx->num_writes = 1;
            tctx->operations[tctx->oper_count++] = oper_list[SYNC][R];
            tctx->num_reads = 1;
            tctx->operations[tctx->oper_count++] = oper_list[SYNC][C];
            tctx->compare_enabled = 'y';
            tctx->operations[tctx->oper_count++] = oper_list[tctx->testcase][R_CACHE];
            tctx->operations[tctx->oper_count++] = oper_list[tctx->testcase][C_CACHE];
	    } else {
            tctx->operations[tctx->oper_count++] = oper_list[tctx->testcase][W_CACHE];
            tctx->num_writes = 1;
            tctx->operations[tctx->oper_count++] = oper_list[tctx->testcase][C_CACHE];
	    }
    } else {
        while (i < len) {
            if (tctx->oper[i] == 'W' || tctx->oper[i] == 'w') {
                tctx->operations[tctx->oper_count++] = oper_list[tctx->testcase][W];
                tctx->num_writes = 1;
                w_found = 1;
            } else if (tctx->oper[i] == 'R' || tctx->oper[i] == 'r') {
                tctx->operations[tctx->oper_count++] = oper_list[tctx->testcase][R];
                tctx->num_reads = 1;
                r_found = 1;
            } else if (tctx->oper[i] == 'C' || tctx->oper[i] == 'c' || tctx->oper[i] == 'V') {
                tctx->operations[tctx->oper_count++] = oper_list[tctx->testcase][C];
                tctx->compare_enabled = 'y';
        #ifdef __CAPI_FLASH__
            } else if (tctx->oper[i] == 'U' || tctx->oper[i] == 'u') {
                tctx->operations[tctx->oper_count++] = oper_list[tctx->testcase][U];
        #else
            } else if (tctx->oper[i] == 'D' || tctx->oper[i] == 'd') {
                tctx->operations[tctx->oper_count++] = oper_list[tctx->testcase][D];
        #endif
                tctx->num_discards = 1;
            } else if (tctx->oper[i] == '[') {
                j = i + 1;
                k = 0;
                while ( j < len) {
                    if (tctx->oper[j] != ']') {
                        num[k] = tctx->oper[j];
                        k++;
                    } else {
                        i = j;
                        break;
                    }
                    j++;
                }
                num[k] = '\0';
                if (k == len) {
                    return (-1);
                }
                if (w_found) {
                    tctx->num_writes = atoi(num);
                    w_found = 0;
                } else if (r_found) {
                    tctx->num_reads = atoi(num);
                    r_found = 0;
                }
            }
            i++;
        }
    }
    return 0;
}

/**********************************************************/
/*  Function to update min_blkno and max_blkno in case    */
/*  section parameter is defined as "yes" in rulefile.    */
/**********************************************************/
void update_blkno(struct htx_data *htx_ds, struct ruleinfo *ruleptr, struct thread_context *tctx)
{
    #ifndef __CAPI_FLASH__
    int th_num, remaining_blks;
    unsigned long long num_blks_per_thread;
    #endif
    struct thread_context *current_tctx;

    if (ruleptr->BWRC_th_mem_index != -1) {
        current_tctx = tctx + ruleptr->BWRC_th_mem_index;
    } else {
        current_tctx = tctx;
    }
	#ifdef __CAPI_FLASH__
	current_tctx->min_blkno =  ruleptr->min_blkno.value[0];
	current_tctx->max_blkno = chunk_size - 1;
	#else
    num_blks_per_thread = (ruleptr->max_blkno.value[0] - ruleptr->min_blkno.value[0] + 1) / ruleptr->num_threads;
    /* DPRINT("num_blks_per_thread: %lld\n", num_blks_per_thread); */
    remaining_blks = (ruleptr->max_blkno.value[0] - ruleptr->min_blkno.value[0] + 1) % ruleptr->num_threads;
    for (th_num = 0; th_num < ruleptr->num_threads; th_num++, current_tctx++) {
        current_tctx->min_blkno = th_num * num_blks_per_thread;
        current_tctx->max_blkno = current_tctx->min_blkno + num_blks_per_thread - 1;
        if (th_num == (ruleptr->num_threads - 1) && remaining_blks != 0) {
            current_tctx->max_blkno += remaining_blks;
        }
#if 0
        /* No need as it is getting called in populate thread_context after update_blkno */
        /* initialize first block and fencepost structure again as these are dependant on min_blkno and max_blkno */
        current_tctx->first_blk = set_first_blk(htx_ds, current_tctx);
        /* DPRINT("id: %s - First block: 0x%llx, min_blkno: 0x%llx, max_blkno: 0x%llx\n", current_tctx->id, current_tctx->first_blk, current_tctx->min_blkno, current_tctx->max_blkno); */
        if (current_tctx->fencepost_index != -1) {
            initialize_fencepost(current_tctx);
        }
#endif
    }
	#endif
}

/************************************************/
/*  Function to initialize fencepost structure  */
/************************************************/
void initialize_fencepost(struct thread_context *tctx)
{
    lba_fencepost[tctx->fencepost_index].direction = tctx->direction;
    if (tctx->direction == UP) {
        lba_fencepost[tctx->fencepost_index].min_lba = tctx->min_blkno;
        lba_fencepost[tctx->fencepost_index].max_lba = tctx->min_blkno;
    } else if (tctx->direction == DOWN) {
        lba_fencepost[tctx->fencepost_index].min_lba = tctx->max_blkno;
        lba_fencepost[tctx->fencepost_index].max_lba = tctx->max_blkno;
    }
    lba_fencepost[tctx->fencepost_index].last_update_time = time(0);
    lba_fencepost[tctx->fencepost_index].status = 'F';
}

/************************************************************/
/*  Function to populate all BWRC thread context from       */
/*  template/ruleinfo. Also, initialize the lba_fencepost   */
/*  array for all BWRC threads                              */
/************************************************************/
int populate_BWRC_thread_context(struct htx_data *htx_ds, struct ruleinfo *ruleptr, struct thread_context *BWRC_tctx)
{
    int th_num, i, index, cur_tmplt_index = 0;
    struct thread_context *current_tctx;

#if 0
    /* If ruleptr->section is YES, update max_blkno and min_blkno for each thread */
    if (ruleptr->section == YES) {
        update_blkno(htx_ds, ruleptr, BWRC_tctx);
    }
#endif

    if (ruleptr->num_associated_templates != 0) {  /* If templates defined */
        for (th_num = 0; th_num < ruleptr->num_threads; ) {
            if (strcasecmp(ruleptr->tmplt[cur_tmplt_index].tmplt_ptr->oper, "BWRC") == 0) {
                if (ruleptr->BWRC_th_mem_index == -1) {
                    ruleptr->BWRC_th_mem_index = free_BWRC_th_mem_index;

                    /* If ruleptr->section is YES, update max_blkno and min_blkno for each thread */
                    if (ruleptr->section == YES) {
                        update_blkno(htx_ds, ruleptr, BWRC_tctx);
				    }
                }
                for (i = 0; i < ruleptr->tmplt[cur_tmplt_index].num_threads; i++) {
                    current_tctx = &(BWRC_tctx[free_BWRC_th_mem_index + i]);
                    current_tctx->fencepost_index = free_BWRC_th_mem_index + i;
                    apply_template_settings(current_tctx, ruleptr->tmplt[cur_tmplt_index].tmplt_ptr);
                    apply_rule_settings(htx_ds, current_tctx, BWRC_tctx, ruleptr, th_num);
                    initialize_fencepost(current_tctx);
                    th_num++;
                }
                free_BWRC_th_mem_index += ruleptr->tmplt[cur_tmplt_index].num_threads;
            } else {
                th_num += ruleptr->tmplt[cur_tmplt_index].num_threads;
            }
            cur_tmplt_index++;
        }
    } else {
        for (th_num = 0; th_num < ruleptr->num_threads; th_num++) {
            if (ruleptr->associativity == SEQ) {
                index = SEQ_INDEX(th_num, ruleptr->num_oper_defined);
            } else {
                index = RROBIN_INDEX(th_num, ruleptr->num_oper_defined);
            }
            if (strcasecmp(ruleptr->oper[index], "BWRC") == 0) {
                if (ruleptr->BWRC_th_mem_index == -1) {
                    ruleptr->BWRC_th_mem_index = free_BWRC_th_mem_index;

                    /* If ruleptr->section is YES, update max_blkno and min_blkno for each thread */
                    if (ruleptr->section == YES) {
                        update_blkno(htx_ds, ruleptr, BWRC_tctx);
				    }
                }
                current_tctx = &(BWRC_tctx[free_BWRC_th_mem_index]);
                current_tctx->fencepost_index = free_BWRC_th_mem_index;
                free_BWRC_th_mem_index++;
                strcpy(current_tctx->oper, ruleptr->oper[index]);
                apply_rule_settings(htx_ds, current_tctx, BWRC_tctx, ruleptr, th_num);
                initialize_fencepost(current_tctx);
            }
        }
    }
    /* for(i=0; i < total_BWRC_threads; i++) {
        DPRINT("lba_fencepost[%d] - min_lba: 0x%llx, max_lba: 0x%llx\n", i, lba_fencepost[i].min_lba, lba_fencepost[i].max_lba);
    } */
    return 0;
}

/****************************************************/
/* Function to update BWRC zone of non-BWRC threads.*/
/* If BWRC threads are defined and oper. is of RC   */
/* type, we need to restrict thread to 1 BWRC zone. */
/****************************************************/
void update_BWRC_zone(struct htx_data *htx_ds, struct thread_context *tctx, struct thread_context *BWRC_tctx)
{
    int i;
    char msg[512];

    if (tctx->direction == UP) {
        for (i=0; i < total_BWRC_threads; i++) {
            if (tctx->min_blkno >= BWRC_tctx[i].min_blkno && tctx->min_blkno <= BWRC_tctx[i].max_blkno) {
                tctx->BWRC_zone_index = i;
                /* If the range of current tctx does not lie in 1 BWRC zone, change max_blkno
                 * to restrict it to 1 BWRC zone.
                 */
                if (tctx->max_blkno > BWRC_tctx[i].max_blkno) {
                    tctx->max_blkno = BWRC_tctx[i].max_blkno;
                }
                break;
            }
        }
    } else if (tctx->direction == DOWN) {
        for (i=0; i < total_BWRC_threads; i++) {
            if (tctx->max_blkno >= BWRC_tctx[i].min_blkno && tctx->max_blkno <= BWRC_tctx[i].max_blkno) {
                tctx->BWRC_zone_index = i;
                /* If the range of current tctx does not lie in 1 BWRC zone, change min_blkno
                 * to restrict it to 1 BWRC zone.
                 */
                if (tctx->min_blkno < BWRC_tctx[i].min_blkno) {
                    tctx->min_blkno = BWRC_tctx[i].min_blkno;
                }
                break;
            }
        }
    } else {
        /* Code should not come here. If here, means direction is either IN or OUT.
         * We don't allocate a BWRC zone in this case. Put a error msg
         */
        sprintf(msg, "if BWRC threads are running, for direction IN/OUT, write operation "
                    "must be defined. ");
        user_msg(htx_ds, 0, 0, HARD, msg);
        exit(1);
    }

    /* It is possible that min_blkno (for UP direction) OR max_blkno (for DOWN
     * direction) does not lie in any of BWRC zone i.e. Due to some gaps, some
     * LBAs are not covered by any BWRC thread. In that case, find the closest
     * BWRC zone.
     */
    if (tctx->BWRC_zone_index == -1) {
        if (tctx->direction == UP) {
            for (i=1; i < total_BWRC_threads; i++) {
                if (tctx->min_blkno > BWRC_tctx[i -1].max_blkno && tctx->min_blkno < BWRC_tctx[i].min_blkno && tctx->max_blkno >= BWRC_tctx[i].min_blkno) {
                    tctx->BWRC_zone_index = i;
                    tctx->min_blkno = BWRC_tctx[i].min_blkno;
                    break;
                }
            }
        } else if (tctx->direction == DOWN) {
            for (i=0; i < total_BWRC_threads - 1; i++) {
                if (tctx->max_blkno < BWRC_tctx[i+1].min_blkno && tctx->max_blkno > BWRC_tctx[i].max_blkno && tctx->min_blkno <= BWRC_tctx[i].max_blkno) {
                    tctx->BWRC_zone_index = i;
                    tctx->max_blkno = BWRC_tctx[i].max_blkno;
                    break;
                }
            }
        }
    }
}
/************************************************************/
/*  Function to populate non-BWRC thread context from the   */
/*  template/ruleinfo structure                             */
/************************************************************/
int populate_thread_context(struct htx_data *htx_ds, struct ruleinfo *ruleptr, struct thread_context *BWRC_tctx, struct thread_context *non_BWRC_tctx)
{
    int i, th_num;
    struct thread_context *current_tctx;
    int non_BWRC_th_mem_index = 0;
    int index, cur_tmplt_index = 0;

    /********************************************************/
    /* Check if only BWRC oper are defined and thread       */
    /* context for it has already been updated in BWRC      */
    /* thread memory. No need to populate it again as BWRC  */
    /* info is maintained for the lifetime of exerciser and */
    /* BWRC_th_mem_index is populated to point the index    */
    /* with in BWRC thread memory for each stanza.          */
    /********************************************************/
    if (ruleptr->is_only_BWRC_stanza == 'Y' && ruleptr->BWRC_th_mem_index != -1) {
        return 0;
    }

    /* If ruleptr->section is YES, update max_blkno and min_blkno for each thread */
    if (ruleptr->section == YES) {
        update_blkno(htx_ds, ruleptr, non_BWRC_tctx);
    }

    /*****************************************************/
    /** If there are templates associated with the rule,**/
    /** First apply template settings and then rest of  **/
    /** rule settings. We need to populate the thread   **/
    /** context based on the operation defined.         **/
    /*****************************************************/
    if (ruleptr->num_associated_templates != 0) {  /* If templates defined */
        for (th_num = 0; th_num < ruleptr->num_threads; ) {
            if (strcasecmp(ruleptr->tmplt[cur_tmplt_index].tmplt_ptr->oper, "BWRC") != 0) {
                for (i = 0; i < ruleptr->tmplt[cur_tmplt_index].num_threads; i++) {
                    current_tctx = &(non_BWRC_tctx[non_BWRC_th_mem_index]);
                    apply_template_settings(current_tctx, ruleptr->tmplt[cur_tmplt_index].tmplt_ptr);
                    apply_rule_settings(htx_ds, current_tctx, BWRC_tctx, ruleptr, th_num);
                    non_BWRC_th_mem_index++;
                    th_num++;
                }
            } else {
                th_num += ruleptr->tmplt[cur_tmplt_index].num_threads;
            }
            cur_tmplt_index++;
        }
    } else { /* else for template condition check */
        for (th_num = 0; th_num < ruleptr->num_threads; th_num++) {
            if (ruleptr->associativity == RROBIN) {
                index = RROBIN_INDEX(th_num, ruleptr->num_oper_defined);
            } else {
                index = SEQ_INDEX(th_num, ruleptr->num_oper_defined);
            }

            if (strcasecmp(ruleptr->oper[index], "BWRC") != 0) {
                current_tctx = &(non_BWRC_tctx[non_BWRC_th_mem_index]);
                strcpy(current_tctx->oper, ruleptr->oper[index]);
                /* Now apply rule setting to the current thread context */
                apply_rule_settings(htx_ds, current_tctx, BWRC_tctx, ruleptr, th_num);
                non_BWRC_th_mem_index++;
            }
        }
    }

    /* for(i=0; i < total_BWRC_threads; i++) {
        DPRINT("lba_fencepost[%d] - min_lba: 0x%llx, max_lba: 0x%llx\n", i, lba_fencepost[i].min_lba, lba_fencepost[i].max_lba);
    } */
    return 0;
}

/************************************************************/
/*  Function to copy template info to the thread context    */
/************************************************************/
void apply_template_settings (struct thread_context *current_tctx, template *tmplt)
{
    strcpy(current_tctx->oper, tmplt->oper);
    current_tctx->seek_breakup_prcnt = tmplt->seek_breakup_prcnt;
    current_tctx->transfer_sz.min_len = tmplt->data_len.min_len;
    current_tctx->transfer_sz.max_len = tmplt->data_len.max_len;
    current_tctx->transfer_sz.increment = tmplt->data_len.increment;
}

/************************************************************/
/*  Function to copy rule info to thread context structure  */
/*  If any template is associated with rule and tempalte    */
/*  variables values are defined in rule stanza also, Then  */
/*  it will over-write template values.                     */
/************************************************************/
void apply_rule_settings (struct htx_data *htx_ds, struct thread_context *current_tctx, struct thread_context *BWRC_tctx, struct ruleinfo *ruleptr, int th_num)
{
    int j;
    char str[32], msg[512];

    /***************************************************************/
    /****       Update thread context from rule stanza          ****/
    /***************************************************************/
    strcpy(current_tctx->id, ruleptr->rule_id);
    sprintf(str, "_%d", th_num);
    strcat(current_tctx->id, str);

    current_tctx->testcase = ruleptr->testcase;
    #ifdef __CAPI_FLASH__
        current_tctx->open_func 	= open_lun;
        current_tctx->close_func 	= close_lun;
		current_tctx->open_flag 	= ruleptr->open_flag;
    #else
        if (ruleptr->testcase == PASSTHROUGH) {
            current_tctx->open_func 	= &open_passth_disk;
            current_tctx->close_func 	= &close_passth_disk;
        } else {
            current_tctx->open_func 	= &open_disk;
            current_tctx->close_func 	= &close_disk;
        }
    #endif

    /* Populate all operation function according to the oper defined */
    populate_operations(current_tctx);

    current_tctx->mode = ruleptr->mode;
    current_tctx->rule_option = ruleptr->rule_option;

    switch (ruleptr->associativity) {
        case RROBIN:
            strcpy(current_tctx->pattern_id, ruleptr->pattern_id[RROBIN_INDEX(th_num, ruleptr->num_pattern_defined)]);
            if (ruleptr->data_len.size[RROBIN_INDEX(th_num, ruleptr->data_len.num_keywds)].min_len != UNDEFINED) {
                current_tctx->transfer_sz.min_len = ruleptr->data_len.size[RROBIN_INDEX(th_num, ruleptr->data_len.num_keywds)].min_len;
                current_tctx->transfer_sz.max_len = ruleptr->data_len.size[RROBIN_INDEX(th_num, ruleptr->data_len.num_keywds)].max_len;
                current_tctx->transfer_sz.increment = ruleptr->data_len.size[RROBIN_INDEX(th_num, ruleptr->data_len.num_keywds)].increment;
            } else if (current_tctx->transfer_sz.min_len == UNDEFINED) {
                current_tctx->transfer_sz.min_len = DEFAULT_NUM_BLKS * dev_info.blksize;
                current_tctx->transfer_sz.max_len = current_tctx->transfer_sz.min_len;
                current_tctx->transfer_sz.increment = 0;
            }

            current_tctx->dlen = current_tctx->transfer_sz.min_len;
            current_tctx->num_blks = current_tctx->dlen / dev_info.blksize;

            current_tctx->starting_block = (long long)ruleptr->starting_block.value[RROBIN_INDEX(th_num, ruleptr->starting_block.num_keywds)];
            current_tctx->direction = ruleptr->direction.value[RROBIN_INDEX(th_num, ruleptr->direction.num_keywds)];
            if (ruleptr->section != YES) { /* if YES, blknos are already updated in update_blkno function */
                current_tctx->min_blkno = ruleptr->min_blkno.value[RROBIN_INDEX(th_num, ruleptr->min_blkno.num_keywds)];
				#ifdef __CAPI_FLASH__
				current_tctx->max_blkno = chunk_size - 1;
				#else
                current_tctx->max_blkno = ruleptr->max_blkno.value[RROBIN_INDEX(th_num, ruleptr->max_blkno.num_keywds)];
				#endif
            }
            if (current_tctx->min_blkno >= current_tctx->max_blkno) {
                sprintf(msg, "For thread with thread_id: %s, min_blkno=%llx defined is greater than OR equal to max_blk=%#llx."
                             "Will not run the test.", current_tctx->id, current_tctx->min_blkno, current_tctx->max_blkno);
                user_msg(htx_ds, 0, 0, HARD, msg);
            }
            current_tctx->blk_hop = ruleptr->blk_hop.value[RROBIN_INDEX(th_num, ruleptr->blk_hop.num_keywds)];

            /* With num_oper "0", only SEQ. operation can be defined. So,
             * change the seek prcnt of thread to be 0.
             */
            if (ruleptr->num_oper.value[RROBIN_INDEX(th_num, ruleptr->num_oper.num_keywds)] == 0) {
                current_tctx->seek_breakup_prcnt = 0;
            } else {
                /* If seek_breakup_prcnt is defined in rule stanza, then this will override the value if defined
                 * any in template.
                 */
                if (ruleptr->seek_breakup_prcnt.value[RROBIN_INDEX(th_num, ruleptr->seek_breakup_prcnt.num_keywds)] != UNDEFINED) {
                    current_tctx->seek_breakup_prcnt = ruleptr->seek_breakup_prcnt.value[RROBIN_INDEX(th_num, ruleptr->seek_breakup_prcnt.num_keywds)];
                } else if (current_tctx->seek_breakup_prcnt == UNDEFINED) {
                    current_tctx->seek_breakup_prcnt = DEFAULT_SEEK_PRCNT;
                }
            }

            /* Need to get the BWRC zone of all non-BWRC threads where they should do IO opers. This is needed only if
             * there are SEQ opers, num_writes are 0 and comapre is enabled i.e RC kind of operation.
             */
            if ((strcasecmp(current_tctx->oper, "BWRC") != 0) && total_BWRC_threads != 0 && current_tctx->seek_breakup_prcnt != 100 &&
                current_tctx->num_writes == 0 && current_tctx->compare_enabled == 'y') {
                update_BWRC_zone(htx_ds, current_tctx, BWRC_tctx);
            }

            current_tctx->align = ruleptr->align.value[RROBIN_INDEX(th_num, ruleptr->align.num_keywds)];
            current_tctx->lba_align = ruleptr->lba_align.value[RROBIN_INDEX(th_num, ruleptr->lba_align.num_keywds)];
            check_alignment(htx_ds, current_tctx);

            /* Set first block if there are SEQ oper defined */
            if (current_tctx->seek_breakup_prcnt != 100) {
                current_tctx->first_blk = set_first_blk(htx_ds, current_tctx);
            }

            /**********************************************/
            /***    if num_oper is zero, check partial  ***/
            /***    transfer and update num_oper        ***/
            /**********************************************/
            if (ruleptr->num_oper.value[RROBIN_INDEX(th_num, ruleptr->num_oper.num_keywds)] == 0) {
                update_num_oper(current_tctx);
             } else {
                if (current_tctx->seek_breakup_prcnt == 0 || current_tctx->seek_breakup_prcnt == 100) {
                    if (current_tctx->seek_breakup_prcnt == 0) {
                        current_tctx->num_oper[SEQ] = ruleptr->num_oper.value[RROBIN_INDEX(th_num, ruleptr->num_oper.num_keywds)];
                    } else {
                        current_tctx->num_oper[RANDOM] = ruleptr->num_oper.value[RROBIN_INDEX(th_num, ruleptr->num_oper.num_keywds)];
                    }
                } else {
                    current_tctx->num_oper[RANDOM] = (ruleptr->num_oper.value[RROBIN_INDEX(th_num, ruleptr->num_oper.num_keywds)] * current_tctx->seek_breakup_prcnt) / 100;
                    current_tctx->num_oper[SEQ] = ruleptr->num_oper.value[RROBIN_INDEX(th_num, ruleptr->num_oper.num_keywds)]  - current_tctx->num_oper[RANDOM];
                }
            }

            current_tctx->hotness = ruleptr->hotness.value[RROBIN_INDEX(th_num, ruleptr->hotness.num_keywds)];
            current_tctx->offset = ruleptr->loop_on_offset.value[RROBIN_INDEX(th_num, ruleptr->loop_on_offset.num_keywds)];
            current_tctx->num_mallocs = ruleptr->num_mallocs.value[RROBIN_INDEX(th_num, ruleptr->num_mallocs.num_keywds)];

            if (ruleptr->rule_option & USER_SEEDS_FLAG) {
                for (j = 0; j < 3; j++) {
                    current_tctx->seed.xsubi[j] = ruleptr->user_defined_seed[j];
                    current_tctx->data_seed.xsubi[j] = ruleptr->user_defined_seed[j+3];
                }
                current_tctx->lba_seed = ruleptr->user_defined_lba_seed;
            }
            current_tctx->run_reread = ruleptr->run_reread;
            current_tctx->num_cache_threads = ruleptr->num_cache_threads;

            current_tctx++;
            break;

        case SEQ:
            strcpy(current_tctx->pattern_id, ruleptr->pattern_id[SEQ_INDEX(th_num, ruleptr->num_pattern_defined)]);
            if (ruleptr->data_len.size[SEQ_INDEX(th_num, ruleptr->data_len.num_keywds)].min_len != UNDEFINED) {
                current_tctx->transfer_sz.min_len = ruleptr->data_len.size[SEQ_INDEX(th_num, ruleptr->data_len.num_keywds)].min_len;
                current_tctx->transfer_sz.max_len = ruleptr->data_len.size[SEQ_INDEX(th_num, ruleptr->data_len.num_keywds)].max_len;
                current_tctx->transfer_sz.increment = ruleptr->data_len.size[SEQ_INDEX(th_num, ruleptr->data_len.num_keywds)].increment;
            } else if (current_tctx->transfer_sz.min_len == UNDEFINED) {
                current_tctx->transfer_sz.min_len = DEFAULT_NUM_BLKS * dev_info.blksize;
                current_tctx->transfer_sz.max_len = current_tctx->transfer_sz.min_len;
                current_tctx->transfer_sz.increment = 0;
            }

            current_tctx->dlen = current_tctx->transfer_sz.min_len;
            current_tctx->num_blks = current_tctx->dlen / dev_info.blksize;

            current_tctx->starting_block = (long long)ruleptr->starting_block.value[SEQ_INDEX(th_num, ruleptr->starting_block.num_keywds)];
            current_tctx->direction = ruleptr->direction.value[SEQ_INDEX(th_num, ruleptr->direction.num_keywds)];
            if (ruleptr->section != YES) {
                current_tctx->min_blkno = ruleptr->min_blkno.value[SEQ_INDEX(th_num, ruleptr->min_blkno.num_keywds)];
				#ifdef __CAPI_FLASH__
				current_tctx->max_blkno = chunk_size - 1;
				#else
                current_tctx->max_blkno = ruleptr->max_blkno.value[SEQ_INDEX(th_num, ruleptr->max_blkno.num_keywds)];
				#endif
            }
            if (current_tctx->min_blkno >= current_tctx->max_blkno) {
                sprintf(msg, "For thread with thread_id: %s, min_blkno=%llx defined is greater than OR equal to max_blk=%#llx."
                            " Will not run the test.", current_tctx->id, current_tctx->min_blkno, current_tctx->max_blkno);
                user_msg(htx_ds, 0, 0, HARD, msg);
		    }
            current_tctx->blk_hop = ruleptr->blk_hop.value[SEQ_INDEX(th_num, ruleptr->blk_hop.num_keywds)];

            /* With num_oper "0", only SEQ. operation can be defined. So,
             * change the seek prcnt of thread to be 0.
             */
            if (ruleptr->num_oper.value[SEQ_INDEX(th_num, ruleptr->num_oper.num_keywds)] == 0) {
                current_tctx->seek_breakup_prcnt = 0;
            } else {
                /* If seek_breakup_prcnt is defined in rule stanza, then this will override the value if defined
                 * any in template.
                 */
                if (ruleptr->seek_breakup_prcnt.value[SEQ_INDEX(th_num, ruleptr->seek_breakup_prcnt.num_keywds)] != UNDEFINED) {
					current_tctx->seek_breakup_prcnt = ruleptr->seek_breakup_prcnt.value[SEQ_INDEX(th_num, ruleptr->seek_breakup_prcnt.num_keywds)];
                } else if (current_tctx->seek_breakup_prcnt == UNDEFINED) {
                    current_tctx->seek_breakup_prcnt = DEFAULT_SEEK_PRCNT;
                }
            }

            /* Need to get the BWRC zone of all non-BWRC threads where they should do IO opers. This is needed only if
             * there are SEQ opers, num_writes are 0 and comapre is enabled i.e RC kind of operation.
             */
            if ((strcasecmp(current_tctx->oper, "BWRC") != 0) && total_BWRC_threads != 0 && current_tctx->seek_breakup_prcnt != 100 &&
                current_tctx->num_writes == 0 && current_tctx->compare_enabled == 'y') {
                update_BWRC_zone(htx_ds, current_tctx, BWRC_tctx);
            }

            current_tctx->align = ruleptr->align.value[SEQ_INDEX(th_num, ruleptr->align.num_keywds)];
            current_tctx->lba_align = ruleptr->lba_align.value[SEQ_INDEX(th_num, ruleptr->lba_align.num_keywds)];
            check_alignment(htx_ds, current_tctx);
            /* Set first block if there are SEQ oper defined */
            if (current_tctx->seek_breakup_prcnt != 100) {
                current_tctx->first_blk = set_first_blk(htx_ds, current_tctx);
		    }

            /**********************************************/
            /***    if num_oper is zero, check partial  ***/
            /***    transfer and update num_oper        ***/
            /**********************************************/
            if (ruleptr->num_oper.value[RROBIN_INDEX(th_num, ruleptr->num_oper.num_keywds)] == 0) {
                update_num_oper(current_tctx);
            } else {
                if (current_tctx->seek_breakup_prcnt == 0 || current_tctx->seek_breakup_prcnt == 100) {
                    if (current_tctx->seek_breakup_prcnt == 0) {
                        current_tctx->num_oper[SEQ] = ruleptr->num_oper.value[SEQ_INDEX(th_num, ruleptr->num_oper.num_keywds)];
                    } else {
                        current_tctx->num_oper[RANDOM] = ruleptr->num_oper.value[SEQ_INDEX(th_num, ruleptr->num_oper.num_keywds)];
                    }
                } else {
                    current_tctx->num_oper[RANDOM] = (ruleptr->num_oper.value[SEQ_INDEX(th_num, ruleptr->num_oper.num_keywds)] * current_tctx->seek_breakup_prcnt) / 100;
                    current_tctx->num_oper[SEQ] = ruleptr->num_oper.value[SEQ_INDEX(th_num, ruleptr->num_oper.num_keywds)]  - current_tctx->num_oper[SEQ];
                }
            }

            current_tctx->hotness = ruleptr->hotness.value[SEQ_INDEX(th_num, ruleptr->hotness.num_keywds)];
            current_tctx->offset = ruleptr->loop_on_offset.value[SEQ_INDEX(th_num, ruleptr->loop_on_offset.num_keywds)];
            current_tctx->num_mallocs = ruleptr->num_mallocs.value[SEQ_INDEX(th_num, ruleptr->num_mallocs.num_keywds)];

            if (ruleptr->rule_option & USER_SEEDS_FLAG) {
                for (j = 0; j < 3; j++) {
                    current_tctx->seed.xsubi[j] = ruleptr->user_defined_seed[j];
                    current_tctx->data_seed.xsubi[j] = ruleptr->user_defined_seed[j+3];
                }
                current_tctx->lba_seed = ruleptr->user_defined_lba_seed;
            }
            current_tctx->run_reread = ruleptr->run_reread;
            current_tctx->num_cache_threads = ruleptr->num_cache_threads;

            current_tctx++;

            break;
    }
}

#ifndef __HTX_LINUX__
/************************************************/
/* Function to check if disk exerciser can be   */
/* on device specified.                         */
/************************************************/
int check_disk(unsigned char *diskdev, char *sysmsg, int msg_length)
{
    char pvname[32], search_str[80], *errmsg, pvid_str[33];
    int term_rc, rc = 0;
    struct CuAt CuAt_obj, *p_CuAt;
    struct querypv *p_qpv;  /* querypv structure */
    struct unique_id pv_id, vg_id;

    strcpy( pvname, &(diskdev[6]) );

    /* init odm */
    rc = (int) odm_initialize();
    if (rc < 0) {
        odm_err_msg(odmerrno, (char **) &errmsg);
        strncpy(sysmsg, errmsg, msg_length);
        sysmsg[msg_length-1] = '\0';
        return(odmerrno);
    }

    /* build the query */
    /* Find the CuAt object corresponding to the pvid for that disk */
    sprintf(search_str, "name = '%s' and attribute = 'pvid'", pvname);
    p_CuAt = odm_get_first(CuAt_CLASS, search_str,&CuAt_obj);
    if ( (int)p_CuAt == -1 ) {
        odm_err_msg(odmerrno, (char **) &errmsg);
        strncpy(sysmsg, errmsg, msg_length);
        sysmsg[msg_length-1] = '\0';
        return (odmerrno);
    } else if (p_CuAt == NULL) {    /* no pvid found for disk device */
        rc = -1;
    }

    if ( rc == 0 ) {
        /* convert pvid to lvm structure */
        strncpy(pvid_str, CuAt_obj.value, 16); /* only use first 16 digits */
        pvid_str[16]='\0';
        bzero(&pv_id, sizeof(pv_id));
        get_uniq_id(pvid_str, &pv_id);

        /* Find the CuAt object for that disk in a volume group */
        sprintf(search_str, "value = '%s' and attribute = 'pv'", CuAt_obj.value);
        p_CuAt = odm_get_first(CuAt_CLASS, search_str,&CuAt_obj);
        if ( (int)p_CuAt == -1 ) {
            odm_err_msg(odmerrno, (char **) &errmsg);
            strncpy(sysmsg, errmsg, msg_length);
            sysmsg[msg_length-1] = '\0';
            rc = odmerrno;
        } else if (p_CuAt == NULL) {    /* no pv attribute with disk's pv_id */
            rc = -2;
        }
    }
    term_rc = odm_terminate();
    if ( term_rc != 0 ) {
        strncat(sysmsg, "odm_terminate() Failed. ", msg_length - strlen(sysmsg)-1 );
        rc = rc == 0 ? odmerrno : rc;    /* don't clobber former bad rc */
    }

    if (rc != 0) return(rc);

    /* now check to see if the LVM agrees with what we found, this will */
    /* read the PVID off the disk. */
    bzero(&vg_id, sizeof(vg_id));
    rc = lvm_querypv(&vg_id, &pv_id, &p_qpv, pvname);
    switch (rc) {
    case LVM_SUCCESS:    /* disk or could be a member of a VG */
    case LVM_BADBBDIR:
    case LVM_NOPVVGDA:
        free(p_qpv->pp_map);
        free(p_qpv);
        sprintf(sysmsg, "Device appears to be a member of the %s volume group.", CuAt_obj.name);
        rc = 0;
        break;
    case LVM_NOTVGMEM:   /* disk is not actually a member, ODM is screwed up  */
        break;
    default:             /* unexpected error */
        rc = 6000;
        break;
    }
    return rc;
}
#endif

/************************************************/
/* Function to get max block no. and block size */
/* of the disk.                                 */
/************************************************/
int get_disk_info(struct htx_data *data, char *dev_name)
{
    int filedes;
    char msg[256], *d_name, device_name[20];
    struct devinfo info;
    int rc = 0;

    /* get disk name */
    strcpy(device_name, dev_name);
    d_name = basename(device_name);
    if (d_name[0] == 'r') {
        strcpy(dev_info.diskname, &d_name[1]);
    } else {
        strcpy(dev_info.diskname, d_name);
    }

    filedes = open(dev_name, O_RDONLY);
    if (filedes == -1 ) {
        sprintf(msg, "Open error in get_disk_info, dev_name=%s, errno. set is: %d\n", dev_name, errno);
        user_msg(data, errno, 0, HARD, msg);
        return(-1);
    }

    /* Get device info */
#ifdef __HTX_LINUX__
    	rc = htx_ioctl(data, filedes, IOCINFO, (void *) &info);
#else
    	rc = do_ioctl(data, filedes, IOCINFO, &info);
#endif
    if (rc == -1) {
        sprintf(msg, "IOCTL IOCINFO error in get_disk_info.\n");
        user_msg(data, errno, 0, HARD, msg);
        if ((close(filedes)) == -1) {
            sprintf(msg, "CLOSE call failed in get_disk_info.\n");
            user_msg(data, errno, 0, HARD, msg);
        }
        return(-1);
    }

#ifdef __HTX_LINUX__
    dev_info.maxblk = (unsigned long long)info.un.scdk.numblks;
    dev_info.blksize = (unsigned int) info.un.scdk.blksize;
    sprintf(msg, "Device %s Information:  \n" "BlockSize = 0x%x,  Number of Blocks = 0x%llx\n",
            data->sdev_id, dev_info.blksize, dev_info.maxblk);
    user_msg(data, 0, 0, INFO, msg);
#else
    if ( info.devtype == DD_SCDISK || info.devtype == DD_SCRWOPT || info.devtype == DD_CDROM)  {
        rc = get_lun(data, dev_info.diskname);
        if (rc) {
            sprintf(msg, "Unable to retreive SCSI ID/LUN from ODM database. rc=%d", rc);
            user_msg(data, 0, 0, HARD, msg);
            if ((close(filedes)) == -1) {
                sprintf(msg, "CLOASE call failed!");
                user_msg(data, errno, 0, HARD, msg);
            }
            exit(1);
        } else {
            if (dev_info.debug_flag == 1) {
                sprintf(msg, "Device_name: %s, Parent: %s\n", dev_info.diskname, dev_info.disk_parent);
            }
            if (info.devtype == DD_CDROM)  { /* SCSI or IDE CDROMs */
                /* There is no way to distinguish between a SCSI and IDE CDROM as DD_CDROM and DD_SCCD both
                 * are defined as 'C' in devinfo.h. So, the only way to get correct blksize for IDE drive is
                 * to check DF_LGDSK flag (see defect 953428)
                 */
                if (info.flags & DF_LGDSK ) {
                    dev_info.maxblk = ((unsigned long long)info.un.sccd64.hi_numblks << 32) | ((unsigned long long)info.un.sccd64.lo_numblks & 0xffffffffULL);  /* yes */
                    dev_info.blksize = info.un.sccd64.blksize;
			    } else {
                    dev_info.maxblk = (unsigned int)info.un.idecd.numblks;
                    dev_info.blksize = (unsigned int)info.un.idecd.blksize;
			    }
            } else if (info.devtype == DD_SCDISK || info.devtype == DD_SCRWOPT) { /* SCSI or IDE disks */
                dev_info.maxblk = ((unsigned long long)info.un.scdk64.hi_numblks << 32) | ((unsigned long long)info.un.scdk64.lo_numblks & 0xffffffffULL);  /* yes */
                dev_info.blksize = info.un.scdk64.blksize;
            }
        }
    } else { /* LV */
        dev_info.maxblk = ((unsigned long long)info.un.dk64.hi_numblks << 32) | ((unsigned long long)info.un.dk64.lo_numblks & 0xffffffffULL);  /* yes */
        dev_info.blksize = info.un.dk64.bytpsec;
    }
    sprintf(msg, "Device %s Information: \n" "Type=%#x, Flag=%#x, \nBlockSize = %#x,  Number of Blocks = %#llx\n",
                data->sdev_id, info.devtype, info.flags, dev_info.blksize, dev_info.maxblk);
    user_msg(data, 0, 0, INFO, msg);
    /* Removed get_dev_fn fr LA trigger */
#endif

    if (dev_info.maxblk == 0) {
        sprintf(msg,"Not able to detect num_blks on disk = %s, maxblk = 0x%llx \n",data->sdev_id, dev_info.maxblk);
        user_msg(data, 0, 0, HARD, msg);
		if ((close(filedes)) == -1) {
        	sprintf(msg, "CLOASE call failed!");
        	user_msg(data, errno, 0, HARD, msg);
        }

        return(-1);
    }
    /* Since numblks tells total blks on disk ranging from 0 to (numblks -1), so, reducing it by 1 */
    dev_info.maxblk--;

	rc = close(filedes);
	if(rc == -1) {
		sprintf(msg, "CLOASE call failed!");
        user_msg(data, errno, 0, HARD, msg);
    }

    return 0;
}

#ifdef __HTX_LINUX__
/**************************************************/
/** Function to check if write cache is enabled  **/
/** for the device. Returns 1 if enabled, other  **/
/** wise 0.                                      **/
/**************************************************/
int check_write_cache(struct htx_data *data)
{
    int fd, WCE_bit = 0;
    char msg[256];

    fd = open(data->sdev_id, O_RDWR);
    if (fd == -1) {
        sprintf(msg, "Error opening device %s for checking if write_cache is enabled. errno: %d\n"
                     "Will not run sync_cache thread", data->sdev_id, errno);
        hxfmsg(data, 0, HTX_HE_INFO, msg);
        return (0);
    }

    WCE_bit = htx_ioctl(data, fd, CHECK_WRITE_CACHE, NULL);
    close(fd);
    return WCE_bit;
}
#endif

/************************************************/
/*  Function to initialize the state table i.e. */
/*  Read the state table from the disk if table */
/*  is VALID (stored as state table metadata on */
/*  the disk itself). Otherwise, build state    */
/*  table from scratch (i.e. write state as 0). */
/************************************************/
int initialize_state_table (struct htx_data *data, char *dev_name)
{

    unsigned long long size;
    int err_no, fd, rc = 0, addr;
    char msg[128], buf[4096];

    /* calculate no. of blocks required to keep the write status of each block on disk.
     * No. should be in multiple of blocks.
     */
    s_tbl.size = ((dev_info.maxblk + 1) * BITS_PER_BLOCK) / BITS_PER_BYTE;
    if (s_tbl.size % dev_info.blksize != 0) {
        s_tbl.size = s_tbl.size + (dev_info.blksize - (s_tbl.size % dev_info.blksize));
    }
    s_tbl.blk_write_status = malloc(s_tbl.size);
    if (s_tbl.blk_write_status == NULL) {
        err_no = errno;
        sprintf(msg, "malloc failed(with errno: %d) while allocating memory for state table info.\n", err_no);
        user_msg(data, err_no, 0, HARD, msg);
        exit(1);
    }
    DPRINT("buffer addr: 0x%llx\n", (unsigned long long)s_tbl.blk_write_status);
    bzero(s_tbl.blk_write_status, s_tbl.size);

    /* Open the device and read block #0 which has info about state table */
    fd = open(dev_name, O_RDWR);
    if (fd == -1) {
        sprintf(msg, "Open error in initialize_state_table. errno. set is: %d\n", errno);
        user_msg(data, errno, 0, HARD, msg);
        return(-1);
    }
    addr = STATE_TABLE_INFO_BLK * dev_info.blksize;
    rc = pread(fd, buf, dev_info.blksize, (offset_t)addr);
    if (rc == -1) {
        sprintf(msg, "Read error while getting state table info in initialize_state_table. errno: %d\n", errno);
        user_msg(data, errno, 0, HARD, msg);
        close(fd);
        return(-1);
    }

    /* update state_table fields */
    s_tbl.status = *(char *) (buf + STATUS_OFFSET);
    if (s_tbl.status == VALID) {
        s_tbl.enablement_time = *(unsigned int *) (buf + TIME_OFFSET);
        strncpy(s_tbl.diskname, buf + DEV_NAME_OFFSET, DEV_NAME_LEN);
        strncpy(s_tbl.hostname, buf + HOSTNAME_OFFSET, HOSTNAME_LEN);

        /* Now, read each block write status into state_table.blk_write_status */
        addr += dev_info.blksize;
        size = * (unsigned long long *) (buf + TABLE_SIZE_OFFSET);
        if (size > s_tbl.size) {
            size = s_tbl.size;
        }
        rc = pread(fd, s_tbl.blk_write_status , size, (offset_t)addr);
        if (rc == -1) {
            sprintf(msg, "Read error for blk_write_status in initialize_state_table. errno: %d\n", errno);
            user_msg(data, errno, 0, HARD, msg);
            close(fd);
            return (-1);
        }

        /* Update the status of state table on disk as INVALID. At the end of the
         * exerciser, we will write back the whole state table and make the status
         * as VALID. Any sudden crash of exercsier, will keep the state table as
         * INVALID so that when exerciser is run next time, it will build state table
         * from scratch.
         */
        *(char *) (buf + STATUS_OFFSET) = INVALID;
        addr = STATE_TABLE_INFO_BLK;
        rc = pwrite (fd, buf, dev_info.blksize, (offset_t)addr);
        if (rc == -1) {
            printf(msg, "Write error in initialize_state_table while updating state table status. errno: %d\n", errno);
            user_msg(data, errno, 0, HARD, msg);
            close(fd);
            return(-1);
        }
    } else {
        s_tbl.enablement_time = (unsigned int) time_mark;
        strcpy(s_tbl.diskname, dev_info.diskname);
        strcpy(s_tbl.hostname, dev_info.hostname);
    }

    /* Also update the min_blkno for each rule to avoid writing the actual
     * test data on the blocks which are used to maintain state table.
     */
    update_min_blkno();
    close(fd);
    return (rc);
}

/***************************************************************/
/* Function to Sync the state table i.e. move the state table  */
/* on to the disk.                                             */
/***************************************************************/
int sync_state_table(struct htx_data *data, char *dev_name)
{
    int fd, rc = 0, addr;
    char msg[128], buf[4096];

    /* Open the device */
    fd = open(dev_name, O_RDWR);
    if (fd == -1) {
        sprintf(msg, "Open error in sync_state_table. errno. set is: %d\n", errno);
        user_msg(data, errno, 0, HARD, msg);
        return(-1);
    }
    addr = WRITE_STATUS_INFO_BLK * dev_info.blksize;
    rc = pwrite (fd, s_tbl.blk_write_status, s_tbl.size, (offset_t)addr);
    if (rc == -1) {
         sprintf(msg, "pwrite error in sync_state_table. errno. is set: %d\n", errno);
         user_msg(data, errno, 0, HARD, msg);
         close(fd);
         return(-1);
    }

    /* Now, update block 0, which contains info about state table */
    *(char *) (buf + STATUS_OFFSET) = VALID;
    *(unsigned long long *) (buf +  TABLE_SIZE_OFFSET) = s_tbl.size;
    *(unsigned int *) (buf +  TIME_OFFSET) = s_tbl.enablement_time;
    strncpy(buf + DEV_NAME_OFFSET, s_tbl.diskname, DEV_NAME_LEN);
    strncpy(buf + HOSTNAME_OFFSET, s_tbl.hostname, HOSTNAME_LEN);
    addr = STATE_TABLE_INFO_BLK;
    rc = pwrite(fd, buf, dev_info.blksize, (offset_t)addr);
    if (rc == -1) {
        sprintf(msg, "pwrite error in sync_state_table while updating status. errno. is set: %d\n", errno);
        user_msg(data, errno, 0, HARD, msg);
        close(fd);
        return(-1);
    }

    /* Free up malloc buffer for blk_write_status */
    free(s_tbl.blk_write_status);
    close(fd);
    return 0;
}

/*******************************************/
/** Function to cleanup resources at exit **/
/*******************************************/
void cleanup_resources()
{
    int i;
    struct thread_context *tctx;

    if (BWRC_threads_mem != NULL) {
        for (i = 0; i < total_BWRC_threads; i++) {
            tctx = &BWRC_threads_mem[i];
            if (tctx->fd != UNDEFINED) {
                close(tctx->fd);
                tctx->fd = UNDEFINED;
		    }
		    free_pattern_buffers (tctx);
		    free_buffers (&(tctx->num_mallocs), tctx);
	    }
    }
    if (non_BWRC_threads_mem != NULL) {
        for (i = 0; i < num_non_BWRC_threads; i++) {
            tctx = &non_BWRC_threads_mem[i];
            if (tctx->fd != UNDEFINED) {
                close(tctx->fd);
                tctx->fd = UNDEFINED;
		    }
		    free_pattern_buffers (tctx);
		    free_buffers (&(tctx->num_mallocs), tctx);
	    }
    }
    cleanup_threads_mem();
}

/*****************************************/
/** Function to cleanup threads memory  **/
/*****************************************/
void cleanup_threads_mem()
{
    if (BWRC_threads_mem != NULL) {
        free(BWRC_threads_mem);
        BWRC_threads_mem = NULL;
    }
    if (non_BWRC_threads_mem != NULL) {
        free(non_BWRC_threads_mem);
        non_BWRC_threads_mem = NULL;
    }
}

/***************************************************************************/
/** Function to set the signal flag to yes when a signal 30 is received by */
/** the exerciser.                                                         */
/***************************************************************************/
void sig_function(int sig, int code, struct sigcontext *scp)
{
    signal_flag = 'Y';
}

/***************************************************************************/
/** Function to set the signal flag to yes when INT signal is received by  */
/** the exerciser.                                                         */
/***************************************************************************/
void int_sig_function(int sig, int code, struct sigcontext *scp)
{
    int_signal_flag = 'Y';
}

/***************************************************************************/
/** Function to gracefully exit exerciser when SIGTERM is received         */
/***************************************************************************/
void SIGTERM_hdl(int sig, int code, struct sigcontext *scp)
{
    exit_flag = 'Y';
}

/************************************************/
/*  Function to print thread context variables  */
/************************************************/
void print_thread_context(struct thread_context *tctx)
{
    printf("==============================\n");
    printf("thread_id: %s\n", tctx->id);
    printf("pattern_id: %s\n", tctx->pattern_id);
    printf("tid: %llx\n", (unsigned long long)tctx->tid);
    printf("testcase: %d\n", tctx->testcase);
    printf("mode: %d\n", tctx->mode);
    printf("oper: %s, oper_count: %d\n", tctx->oper, tctx->oper_count);
    printf("num_oper[SEQ]: %lld, num_oper[RANDOM]: %lld\n", tctx->num_oper[SEQ], tctx->num_oper[RANDOM]);
    printf("starting block: %lld\n", tctx->starting_block);
    printf("Direction: %d\n", tctx->direction);
    printf("min_len: %lld, max_len: %lld, increment: %d\n", tctx->transfer_sz.min_len, tctx->transfer_sz.max_len, tctx->transfer_sz.increment);
    printf("min_blkno: 0x%llx\n", tctx->min_blkno);
    printf("max_blkno: 0x%llx\n", tctx->max_blkno);
    printf("dlen: %lld\n", tctx->dlen);
    printf("num_blks: %d\n", tctx->num_blks);
    printf("blk_hop: %d\n", tctx->blk_hop);
    printf("num_writes: %d\n", tctx->num_writes);
    printf("num_reads: %d\n", tctx->num_reads);
    printf("compare_enabled: %c\n", tctx->compare_enabled);
    printf("BWRC_zone: %d\n", tctx->BWRC_zone_index);
    printf("first_blk: 0x%llx\n", tctx->first_blk);
    printf("seek_breakup_prcnt: %d\n", tctx->seek_breakup_prcnt);
    printf("lba_align: %d\n", tctx->lba_align);
    printf("hotness: %d\n", tctx->hotness);
    printf("num_mallocs: %d\n", tctx->num_mallocs);
    printf("parent_seed: %d %d %d\n", tctx->seed.xsubi[0], tctx->seed.xsubi[1], tctx->seed.xsubi[2]);
    printf("parent_seed: %d %d %d\n", tctx->data_seed.xsubi[0], tctx->data_seed.xsubi[1], tctx->data_seed.xsubi[2]);
    printf("fencepost_index: %d\n", tctx->fencepost_index);
    printf("run_reread: %c\n", tctx->run_reread);
    printf("do_partial: %d\n", tctx->do_partial);
}

