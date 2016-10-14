/* IBM_PROLOG_BEGIN_TAG */
/*
 * Copyright 2003,2016 IBM International Business Machines Corp.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *       http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or
 * implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
/* IBM_PROLOG_END_TAG */

static char sccsid[] = "@(#)70  1.2  src/htx/usr/lpp/htx/lib/htxmp64/htxmp_new.c, htx_libhtxmp, htxfedora 10/8/10 04:38:36"; 

/*
 * COMPONENT_NAME: htx_libhtxmp 
 *
 * FUNCTIONS:
 *
 * ORIGINS: 27
 *
 * IBM CONFIDENTIAL -- (IBM Confidential Restricted when
 * combined with the aggregated modules for this product)
 * OBJECT CODE ONLY SOURCE MATERIALS
 * (C) COPYRIGHT International Business Machines Corp. 1988, 1990, 1991
 * All Rights Reserved
 *
 * US Government Users Restricted Rights - Use, duplication or
 * disclosure restricted by GSA ADP Schedule Contract with IBM Corp.
 */


#include <stdio.h>
#include <pthread.h>
#include <signal.h>

#ifdef __HTX_LINUX__
#define NULL ((void*)0)
#endif
extern int errno;

#ifndef __HTX_LINUX__
#include <sys/systemcfg.h>
#include <sys/processor.h>
#include <sys/mode.h>
#endif

#include <time.h>
#include "hxihtx64.h"

#include "htx_local.h"
#include "htxmp_proto_new.h"


#include <fcntl.h>

#define DUMP_PATH "/tmp/"


#define MAX_MISCOMPARES 10
#define MAX_MSG_DUMP 20
/* Enable to see more debug data  
#define DEBUG 1
*/


extern int bitindex(int *);
#ifdef DEBUG1
extern struct htx_data *statsp;
char msg[256];
#endif


#define DEFAULT_ATTR_PTR   	    NULL	


/*---------------------------------------------------------------------
*  Once init Blocks
*----------------------------------------------------------------------*/

void hxfupdate_once_init()
{
	pthread_mutexattr_init(&mta);
	pthread_mutexattr_settype(&mta, PTHREAD_MUTEX_RECURSIVE);
	pthread_mutex_init(&hxfupdate_mutex, &mta);
}

void hxfpat_once_init()
{
  pthread_mutex_init(&hxfpat_mutex, DEFAULT_ATTR_PTR);
}

void hxfcbuf_once_init()
{
  pthread_mutex_init(&hxfcbuf_mutex, DEFAULT_ATTR_PTR);
}

void hxfsbuf_once_init()
{
  pthread_mutex_init(&hxfsbuf_mutex, DEFAULT_ATTR_PTR);
}
void hxfbindto_a_cpu_once_init()
{
  pthread_mutex_init(&hxfbindto_a_cpu_mutex, DEFAULT_ATTR_PTR);
}

/*---------------------------------------------------------------------
 *  Thread safe "hxfupdate" function protected by a lock for threads
 *----------------------------------------------------------------------*/

void unblock_stats_th() { 
	int rc ; 
	char msgbuf[100]; 
	rc = pthread_cond_broadcast(&start_thread_cond);
   
    rc = pthread_mutex_unlock(&create_thread_mutex);
	/* Set this flag if all the setup was sucess and user had made request 
	 to use new HTXMP lib. 
    Let us decide whether we are using new mp library */ 
	new_mp = 1 ;
}
	

/*---------------------------------------------------------------------
 * Intialize htxmp resoruces 
 * Function : mp_initialize
 * Input : num_resources 
 * Output : return code -1 in case of error 
 *---------------------------------------------------------------------*/

int 
mp_initialize(int num_resources, struct htx_data * htx_ds) { 

	int i,rc = 0 ; 
	char msg_buf[1024]; 

	/* Save off the exerciser requested num_resources */ 
	num_threads = num_resources; 

	/* Since this call is supposed to me made much before threads are created */ 
	/* Passed htx_ds would be pointer to exer specific shared memory  */ 
	/* keep a pointer to exers htx datastructure, mp lib would use this ponter 
	   for error reporting and time stamp updates */ 
	global_mp_htx_ds = htx_ds ;  

	if ((strcmp (htx_ds->run_type, "REG") != 0) && (strcmp (htx_ds->run_type, "EMC") != 0)) { 
		/* I shouldnt be active in exer standalone runs */ 
		return(0);
	} 
#ifdef DEBUG
	sprintf(msg_buf, " Intialization start .. num_resources = %d, htx_ds=%llx \n",num_resources, htx_ds); 
	hxfmsg(htx_ds, 0, HTX_HE_INFO, msg_buf); 
#endif 

	/* Allocate space for mp_resources */ 
	global_mp_struct = malloc(num_resources * sizeof(mp_struct));  
	if(global_mp_struct == NULL) { 
		sprintf(msg_buf, " HTXMP LIB ERROR : Failed to allocate space for htxmp_struct, errno =  %d \n",errno); 
		hxfmsg(htx_ds, (-1), HTX_HE_SOFT_ERROR, msg_buf); 
		return(-1); 
	}
		 
	/* Intialize mp_struct data structure */ 
	for(i = 0; i < num_resources; i++) { 
		rc = pthread_mutex_init(&global_mp_struct[i].mutex_lock, DEFAULT_ATTR_PTR); 
		if(rc == -1) { 
			sprintf(msg_buf, " HTXMP LIB ERROR : Failed to intialize mutex_lock variable, errno = %d \n", errno); 
			hxfmsg(htx_ds, (-1), HTX_HE_SOFT_ERROR, msg_buf); 
			return(rc); 
		} 
		memset(&global_mp_struct[i].local_htx_ds, 0, sizeof(struct htx_data)); 
		memcpy(&global_mp_struct[i].local_htx_ds, global_mp_htx_ds, sizeof(struct htx_data)); 
        global_mp_struct[i].local_htx_ds.bad_others = 0;
        global_mp_struct[i].local_htx_ds.bad_reads = 0;
        global_mp_struct[i].local_htx_ds.bad_writes = 0;
        global_mp_struct[i].local_htx_ds.bytes_read = 0;
        global_mp_struct[i].local_htx_ds.bytes_writ = 0;
        global_mp_struct[i].local_htx_ds.good_others = 0;
        global_mp_struct[i].local_htx_ds.good_reads = 0;
        global_mp_struct[i].local_htx_ds.good_writes = 0;
        global_mp_struct[i].local_htx_ds.num_instructions = 0;
	} 		
	/* mp_start subroutine is used to provide unique index to each thread created by exer
	 * We would need a global mutex lock to keep it thread safe 
	 * Intialize the mutex lock here
	 */ 
	 pthread_mutex_init(&mutex_start, DEFAULT_ATTR_PTR); 
	 pthread_mutex_init(&create_thread_mutex, DEFAULT_ATTR_PTR);
   	 pthread_cond_init(&create_thread_cond, DEFAULT_ATTR_PTR);
     pthread_cond_init(&start_thread_cond, DEFAULT_ATTR_PTR);
	
	/*  Create a stats update thread, this thread created once would read stats from each 
	 *	threads local stats and send the colated result to htxstats process 
	 *	This thread would take a read lock on mp_struct
	 */   
	
	pthread_attr_init(&stats_th_attr); 
	pthread_attr_setdetachstate(&stats_th_attr, PTHREAD_CREATE_JOINABLE); 
	pthread_attr_setscope(&stats_th_attr, PTHREAD_SCOPE_PROCESS); 

	/* Take Lock to create thread */ 
	rc = pthread_mutex_lock(&create_thread_mutex);
    if (rc) {
        sprintf(msg_buf, "HTXMP LIB ERROR : pthread_mutex_lock failed \n");
		hxfmsg(htx_ds, (-1), HTX_HE_SOFT_ERROR, msg_buf); 
        return (-1);
    }

	rc = pthread_create(&stats_th_tid, &stats_th_attr, (void *(*)(void *))update_stats, (void *)global_mp_htx_ds); 
	if(rc) { 
		sprintf(msg_buf, " HTXMP LIB ERROR : Failed to create stats_th, errno = %d \n", errno); 
		hxfmsg(htx_ds, (-1), HTX_HE_SOFT_ERROR, msg_buf); 
		return(-1); 
	} 

	/* Stats update thread would be created but would wait on create_thread_cond, 
	 * I will unblock this thread when first call to mp_start happens */  
	rc = pthread_cond_wait(&create_thread_cond, &create_thread_mutex);
    if (rc) {
        sprintf(msg_buf, " HTXMP LIB ERROR : pthread_cond wait failed, errno=%d \n", errno);
		hxfmsg(htx_ds, (-1), HTX_HE_SOFT_ERROR, msg_buf); 
      	return(-1);  
    }
#ifdef DEBUG
	sprintf(msg_buf, " Intialization done .. num_resources = %d \n",num_resources); 
	hxfmsg(htx_ds, 0, HTX_HE_INFO, msg_buf); 
#endif 
	/* mp_intialization successfull */ 
	new_mp_initialize = 1; 
	return(0); 
} 
 

/*-------------------------------------------------------------------------
 * THis function is supposed to be called by each exers created thread. 
 * This function will allocate each thread a unique tid. 
 * 
 * Function : mp_start()
 * Input : thread's local htx_data structure 
 * Output : Postive integer if successfull. 
 *        : -1 in error.  
 *-------------------------------------------------------------------------*/ 

int
mp_start(struct htx_data * htx_ds) { 
	  
	int rc = 0; 
	char msg_buf[1024]; 	

	if(!new_mp_initialize) { 
		/* Either mp_initialize failed or we are running under standalone mode */ 
		return(0); 
	} 

	rc = pthread_mutex_lock(&mutex_start); 
	if(rc) { 
		sprintf(msg_buf, "HTXMP LIB ERROR : pthread_mutex_lock failed to take lock on mutex_start errno = %d \n",errno); 
		hxfmsg(htx_ds, (-1), HTX_HE_SOFT_ERROR, msg_buf); 
		return(-1); 
	} 
	th_index = ((th_index + 1 ) % num_threads ); 

	/* I would assume thread hasnt intialized its thread specific htx_ds passed as an argument to this function 
	   Do it as part of library. */ 
	memset(htx_ds, 0, sizeof(struct htx_data)); 
	memcpy(htx_ds,global_mp_htx_ds, sizeof(struct htx_data));
	/* Stats parameters should be set to 0 */ 
	htx_ds->bad_others = 0;
    htx_ds->bad_reads = 0;
    htx_ds->bad_writes = 0;
    htx_ds->bytes_read = 0;
    htx_ds->bytes_writ = 0;
    htx_ds->good_others = 0;
    htx_ds->good_reads = 0;
    htx_ds->good_writes = 0;
    htx_ds->num_instructions = 0;
	/* Populate pthread_id appropriately, so that exer doesnt have do it */ 
	htx_ds->pthread_id = th_index ; 

#ifdef DEBUG
    sprintf(msg_buf, " mp_start index generated = %d, htx_ds->test_id = %d\n",th_index, htx_ds->test_id); 
	hxfmsg(htx_ds, 0, HTX_HE_INFO, msg_buf);
#endif

	if(th_index > num_threads) { 
		sprintf(msg_buf, "HTXMP LIB ERROR : exer error, num threaded created differs from previously requested value. index = %d, num_threads = %d \n",
														th_index, num_threads); 
		hxfmsg(htx_ds, (-1), HTX_HE_SOFT_ERROR, msg_buf); 
		return(-1); 
	}
	rc = pthread_mutex_unlock(&mutex_start);
	if(rc) {
        sprintf(msg_buf, "HTXMP LIB ERROR : pthread_mutex_unlock failed to take unlock on mutex_start errno = %d \n",errno);
		hxfmsg(htx_ds, (-1), HTX_HE_SOFT_ERROR, msg_buf); 
        return(-1);
    }
	
	/* Start the stats update thread, when this function is called first time */ 
	pthread_once (&mpstart_onceblock, unblock_stats_th); 
									
	return(th_index); 
} 
	 

/* This thread function will coalate all the stats from 
 * exer thread, and finally make hxfupdate call
 */ 
void
update_stats(void * arg) { 

	char msgbuf[1024]; 
	struct htx_data exer_stats;  		
	int i = 0, rc = 0; 	
	mp_struct * mp = (mp_struct *) global_mp_struct; 
	struct htx_data * htx_ds = (struct htx_data *) arg; 

	rc = pthread_mutex_lock(&create_thread_mutex);
    if (rc) {
        sprintf(msgbuf, " HTXMP LIB ERROR : update_stats: pthread_mutex_lock failed, errno=%d \n", errno);
		hxfmsg(htx_ds, (-1), HTX_HE_SOFT_ERROR, msgbuf); 
		pthread_exit((void*)-1); 
    }

	/* Notify exerciser that stats thread created successfully */
    rc = pthread_cond_broadcast(&create_thread_cond);
    if (rc) {
        sprintf(msgbuf, "HTXMP LIB ERROR : update_stats: pthread_cond_broadcast failed, errno = %d \n", errno);
		hxfmsg(htx_ds, (-1), HTX_HE_SOFT_ERROR, msgbuf); 
		pthread_exit((void *)-1);
    }

	/* Intialize my local mp data structure */
	memset(&exer_stats, 0, sizeof(struct htx_data));
	memcpy(&exer_stats, global_mp_htx_ds, sizeof(struct htx_data));
    exer_stats.bad_others = 0;
    exer_stats.bad_reads = 0;
    exer_stats.bad_writes = 0;
    exer_stats.bytes_read = 0;
    exer_stats.bytes_writ = 0;
    exer_stats.good_others = 0;
    exer_stats.good_reads = 0;
    exer_stats.good_writes = 0;
    exer_stats.num_instructions = 0;

#ifdef DEBUG
    sprintf(msgbuf, " Stats thread started ..Unblock master. \n");
    hxfmsg(htx_ds, 0, HTX_HE_INFO, msgbuf);
#endif
	
    /* wait for start notification */
    rc = pthread_cond_wait(&start_thread_cond, &create_thread_mutex);
    if (rc) {
        sprintf(msgbuf, "HTXMP LIB ERROR : update_stats: pthread_cond_wait failed, errno = %d \n",errno);
		hxfmsg(htx_ds, (-1), HTX_HE_SOFT_ERROR, msgbuf); 
		pthread_exit((void *)-1);
    }

#ifdef DEBUG
    sprintf(msgbuf, " Stats thread started ..rcvd notification \n"); 
	hxfmsg(htx_ds, 0, HTX_HE_INFO, msgbuf);
#endif

    rc = pthread_mutex_unlock(&create_thread_mutex);
    if (rc) {
        sprintf(msgbuf, "HTXMP LIB ERROR : update_stats: pthread_mutex_unlock failed, errno = %d \n",errno);
		hxfmsg(htx_ds, (-1), HTX_HE_SOFT_ERROR, msgbuf); 
		pthread_exit((void *)-1);
    }
#ifdef DEBUG	
	sprintf(msgbuf, " Stats thread started ... \n"); 
	hxfmsg(htx_ds, 0, HTX_HE_INFO, msgbuf);
#endif
	
	/* Now that we are here, start the update process */
	 while(exit_update_td == 0 ) { 
		/* Freq for update = 13 sec, this wld avoid clash with SUP hxfupdate function */ 
		sleep(13); 
		
		/* We need to wait till exerciser had made first call to hxfupdate
		 * function, 
		 */ 
		 if(!exer_stats_updated)
			continue; 

		/* Take a read lock on thread specific stats and read there stats. 
		 * This function inturn add the individual thread specific stats 
		 * to aggregate exer stats in its local htx_ds  
		 */ 
		 for(i = 0; i < num_threads; i++) { 	
			rc = pthread_mutex_lock(&mp[i].mutex_lock); 
			if(rc) { 
				sprintf(msgbuf, "HTXMP LIB ERROR : update_stats: pthread_mutex_lock failed for i = %d, errno= %d \n",i, errno); 
				hxfmsg(htx_ds, (-1), HTX_HE_SOFT_ERROR, msgbuf); 
				pthread_exit((void *)-1); 
			}			

			exer_stats.bad_others 	+= mp[i].local_htx_ds.bad_others; 
			exer_stats.bad_reads 	+= mp[i].local_htx_ds.bad_reads; 
			exer_stats.bad_writes	+= mp[i].local_htx_ds.bad_writes; 
			exer_stats.bytes_read	+= mp[i].local_htx_ds.bytes_read; 
			exer_stats.bytes_writ   += mp[i].local_htx_ds.bytes_writ; 
			exer_stats.good_others  += mp[i].local_htx_ds.good_others; 
			exer_stats.good_reads   += mp[i].local_htx_ds.good_reads ; 
			exer_stats.good_writes  += mp[i].local_htx_ds.good_writes ; 
			exer_stats.num_instructions += mp[i].local_htx_ds.num_instructions; 
			exer_stats.test_id      = global_mp_htx_ds->test_id;
			/* Done with reading stats, make the values to 0 */ 
			mp[i].local_htx_ds.bad_others = 0;
			mp[i].local_htx_ds.bad_reads = 0; 
			mp[i].local_htx_ds.bad_writes = 0; 
			mp[i].local_htx_ds.bytes_read = 0; 
			mp[i].local_htx_ds.bytes_writ = 0; 
			mp[i].local_htx_ds.good_others = 0; 
			mp[i].local_htx_ds.good_reads = 0; 
			mp[i].local_htx_ds.good_writes = 0; 
			mp[i].local_htx_ds.num_instructions = 0;
		#ifdef DEBUG
			sprintf(msgbuf, "HTXMP LIB stats thread : Reading stats for thread = %d , bytes_reads=0x%llx\n", i, exer_stats.bytes_read);
        	hxfmsg(global_mp_htx_ds, rc, HTX_HE_INFO, msgbuf);
		#endif 
			rc = pthread_mutex_unlock(&mp[i].mutex_lock);
			if(rc) {
                sprintf(msgbuf, "HTXMP LIB ERROR : update_stats: pthread_mutex_unlock failed for i = %d, errno= %d \n",i, errno);
				hxfmsg(htx_ds, (-1), HTX_HE_SOFT_ERROR, msgbuf);	
				pthread_exit((void *)-1);
			}
		} 
		/* Now we can make SUP hxfupdate call */ 
		/* TBD : Read/Write Lock b/w this thread and SUP hxfupdate process */ 
		hxfupdate(UPDATE, &exer_stats); 
		if(exit_update_td ) 
			break; 
	}  
	pthread_exit(0); 
}		

int
mp_destroy( struct htx_data * htx_ds ) { 

	int i, rc;  
    int th_rc;
    void *tresult = (void *)th_rc ; /* thread join status */

	/* Give time for stats thread to exit gracefully */  
	exit_update_td = 1;
	sleep(1); 
	rc = pthread_join(stats_th_tid, &tresult);
	if(rc != 0) { 
		sprintf(htx_ds->msg_text,"pthread_join failed ! errno %d :(%d): tnum=%lu\n", errno, rc, stats_th_tid);
        hxfmsg(htx_ds, HTX_HE_HARD_ERROR , rc , htx_ds->msg_text);
		return(-1); 
	} 

	for(i = 0; i < num_threads; i++) { 
		 pthread_mutex_destroy(&global_mp_struct[i].mutex_lock);
	}
	pthread_attr_destroy(&stats_th_attr); 
    pthread_cond_destroy(&create_thread_cond);
    pthread_cond_destroy(&start_thread_cond);
    pthread_mutex_destroy(&create_thread_mutex);
	pthread_mutex_destroy(&mutex_start); 

	free(global_mp_struct); 
	return(0);
}


int 
hxfupdate(char type, struct htx_data * data) {    

	int rc = 0; 
        int thread_index = 0;
        mp_struct * mp = global_mp_struct;
        long clock;
        char msg_buf[1024];
	int lockinit1, lock_ret, lock_ret_1;
	lockinit1 = lock_ret = lock_ret_1 = 0;
	char    msg_send[MSG_TEXT_SIZE];
        if((new_mp == 1) && (type == UPDATE)) {

                int thread_index = 0;
                mp_struct * mp = global_mp_struct;
                long clock;
                char msg_buf[1024];

                thread_index = data->pthread_id;

                /* Update the timestamp, this would make sure each call  for hxfupdate increments timestamps */
                /*
                clock = time ((long *) 0);
                if (global_mp_htx_ds->p_shm_HE->halt_flag == 0)
                        global_mp_htx_ds->p_shm_HE->run_time += (clock - global_mp_htx_ds->p_shm_HE->tm_last_upd);
                global_mp_htx_ds->p_shm_HE->tm_last_upd = clock;
                */
                if ((strcmp (global_mp_htx_ds->run_type, "REG") != 0) && (strcmp (global_mp_htx_ds->run_type, "EMC") != 0) ) {
                        /* I shouldnt be active in exer standalone runs */
                        return(0);
                }
                /* New version for updates will take a lock per thread instead of global lock */
                rc = pthread_mutex_lock(&mp[thread_index].mutex_lock);
                if(rc) {
                sprintf(msg_buf, "HTXMP LIB ERROR : hxfupdate_tsafe : pthread_mutex_lock failed for i = %d, errno= %d \n", thread_index, errno);
                hxfmsg(global_mp_htx_ds, rc, HTX_HE_HARD_ERROR, msg_buf);
        }
        #ifdef DEBUG
                sprintf(msg_buf, "HTXMP LIB hxfupdate_tsafe : Update stats for thread = %d , bytes_reads=0x%llx\n", thread_index, data->bytes_read);
                hxfmsg(global_mp_htx_ds, rc, HTX_HE_INFO, msg_buf);
        #endif
                /* Update the stats */
                mp[thread_index].local_htx_ds.bad_others        += data->bad_others;
                mp[thread_index].local_htx_ds.bad_reads         += data->bad_reads;
                mp[thread_index].local_htx_ds.bad_writes        += data->bad_writes;
                mp[thread_index].local_htx_ds.bytes_read        += data->bytes_read;
                mp[thread_index].local_htx_ds.bytes_writ        += data->bytes_writ;
                mp[thread_index].local_htx_ds.good_others       += data->good_others;
                mp[thread_index].local_htx_ds.good_reads        += data->good_reads;
                mp[thread_index].local_htx_ds.good_writes       += data->good_writes;
                mp[thread_index].local_htx_ds.num_instructions += data->num_instructions;
                mp[thread_index].local_htx_ds.test_id           = data->test_id;

                rc = pthread_mutex_unlock(&mp[thread_index].mutex_lock);
                if(rc) {
                sprintf(msg_buf, "HTXMP LIB ERROR : hxfupdate_tsafe: pthread_mutex_unlock failed for i = %d, errno= %d \n",thread_index, errno);
                hxfmsg(global_mp_htx_ds, rc, HTX_HE_HARD_ERROR, msg_buf);
        }

                data->bad_others = 0;
                data->bad_reads = 0;
                data->bad_writes = 0;
                data->bytes_read = 0;
                data->bytes_writ = 0;
                data->good_others = 0;
                data->good_reads = 0;
                data->good_writes = 0;
                data->num_instructions = 0;
                /* We had updated the thread specific stats structure,
                 * now stats update thread is good to start
                 */
                 exer_stats_updated = 1;
        }

	else if(type == MESSAGE) {
		/* message logging made lock free, using trylock to stop in case the thread is already blocked*/
		lockinit1 = pthread_mutex_trylock (&hxfupdate_mutex);
        if(lockinit1 == 0){
			/* if lock not already taken, not need to lock as msg logging is lock free, so unlocking*/
			pthread_mutex_unlock (&hxfupdate_mutex);
			rc=htx_message(data,msg_send);		   	
   		 }	
		else if (lockinit1 == EBUSY){
			/*if lock already taken by UPD or ERR call, block the current execution*/

			lock_ret = pthread_mutex_lock(&hxfupdate_mutex);
			if (lock_ret!=0  ) {
			        if (( lock_ret == EAGAIN ) || ( lock_ret == EBUSY ))  {
                       			usleep(10);
		                       	lock_ret_1 = pthread_mutex_lock(&hxfupdate_mutex);
	        		       	if(lock_ret_1!=0){
	        	        		sprintf(msg_buf, "HTXMP LIB ERROR : hxfupdate: pthread_mutex_lock failed under MESSAGE = %d \n",lock_ret_1);
		        	        	hxfmsg(global_mp_htx_ds, rc, HTX_HE_HARD_ERROR, msg_buf);

                		 	}
			        }
		        	else {
 	        			sprintf(msg_buf, "HTXMP LIB ERROR : hxfupdate: pthread_mutex_lock failed under MESSAGE even after retrying = %d \n",lock_ret);
			               	hxfmsg(global_mp_htx_ds, rc, HTX_HE_HARD_ERROR, msg_buf);
			        }
			}
			htx_message(data,msg_send);
			lock_ret = pthread_mutex_unlock (&hxfupdate_mutex);
			if (lock_ret!=0  ) {
				sprintf(msg_buf, "HTXMP LIB ERROR : hxfupdate: pthread_mutex_unlock failed under MESSAGE = %d \n",lock_ret);
				hxfmsg(global_mp_htx_ds, rc, HTX_HE_HARD_ERROR, msg_buf);
			}
			
		}
		else{
                	sprintf(msg_buf, "HTXMP LIB ERROR : hxfupdate_tsafe: pthread_mutex_unlock failed for errno = %d \n",lockinit1);
                	hxfmsg(global_mp_htx_ds, rc, HTX_HE_HARD_ERROR, msg_buf);
		}
   	 }

	else if(type == UPDATE){
		/*in UPD call, we need to block the thread which is currently updating the shm of exerciser*/

		lock_ret = pthread_mutex_lock(&hxfupdate_mutex);
		if (lock_ret!=0  ) {
			if (( lock_ret == EAGAIN ) || ( lock_ret == EBUSY ))  {
				usleep(10);
				lock_ret_1 = pthread_mutex_lock(&hxfupdate_mutex);
				if(lock_ret_1!=0){
					sprintf(msg_buf, "HTXMP LIB ERROR : hxfupdate: pthread_mutex_lock failed under UPDATE = %d \n",lock_ret_1);
					hxfmsg(global_mp_htx_ds, rc, HTX_HE_HARD_ERROR, msg_buf);
				}
			}
			else {
				sprintf(msg_buf, "HTXMP LIB ERROR : hxfupdate: pthread_mutex_lock failed under UPDATE = %d \n",lock_ret);
				hxfmsg(global_mp_htx_ds, rc, HTX_HE_HARD_ERROR, msg_buf);
		       }
		}
                
		hxfupdate_tunsafe((type), (data));

		lock_ret = pthread_mutex_unlock (&hxfupdate_mutex);
		if (lock_ret!=0  ) {
			sprintf(msg_buf, "HTXMP LIB ERROR : hxfupdate: pthread_mutex_unlock failed under MESSAGE = %d \n",lock_ret);
			hxfmsg(global_mp_htx_ds, rc, HTX_HE_HARD_ERROR, msg_buf);
		}

	}

	else if ((type == ERROR) || (type == FINISH) || (type == RECONFIG)){
		/*take lock and make ERR call*/
                lock_ret = pthread_mutex_lock(&hxfupdate_mutex);
                if (lock_ret!=0  ) {
                        if (( lock_ret == EAGAIN ) || ( lock_ret == EBUSY ))  {
                                usleep(10);
                                lock_ret_1 = pthread_mutex_lock(&hxfupdate_mutex);
                                if(lock_ret_1!=0){
                                        sprintf(msg_buf, "HTXMP LIB ERROR : hxfupdate: pthread_mutex_lock failed under ERROR/FINISH/RECONFIG = %d \n",lock_ret_1);
                                        hxfmsg(global_mp_htx_ds, rc, HTX_HE_HARD_ERROR, msg_buf);
                                }
                        }
                	else {
                        	sprintf(msg_buf, "HTXMP LIB ERROR : hxfupdate: pthread_mutex_lock failed under ERROR/FINISH/RECONFIG even after retrying= %d \n",lock_ret);
	                        hxfmsg(global_mp_htx_ds, rc, HTX_HE_HARD_ERROR, msg_buf);
        	       }
		}

                hxfupdate_tunsafe((type), (data));

                lock_ret = pthread_mutex_unlock (&hxfupdate_mutex);
                if (lock_ret!=0  ) {
                        sprintf(msg_buf, "HTXMP LIB ERROR : hxfupdate: pthread_mutex_unlock failed under ERROR/FINISH/RECONFIG = %d \n",lock_ret);
                        hxfmsg(global_mp_htx_ds, rc, HTX_HE_HARD_ERROR, msg_buf);
                }

	}

	else if (type == START){
		hxfupdate_once_init();
                lock_ret = pthread_mutex_lock(&hxfupdate_mutex);
                if (lock_ret!=0  ) {
                        if (( lock_ret == EAGAIN ) || ( lock_ret == EBUSY ))  {
                                usleep(10);
                                lock_ret_1 = pthread_mutex_lock(&hxfupdate_mutex);
                                if(lock_ret_1!=0){
                                        sprintf(msg_buf, "HTXMP LIB ERROR : hxfupdate: pthread_mutex_lock failed under START = %d \n",lock_ret_1);
                                        hxfmsg(global_mp_htx_ds, rc, HTX_HE_HARD_ERROR, msg_buf);
                                }
                        }
                	else {
                        	sprintf(msg_buf, "HTXMP LIB ERROR : hxfupdate: pthread_mutex_lock failed under START even after retrying= %d \n",lock_ret);
	                        hxfmsg(global_mp_htx_ds, rc, HTX_HE_HARD_ERROR, msg_buf);
        	       }
		}
                hxfupdate_tunsafe((type), (data));

                lock_ret = pthread_mutex_unlock (&hxfupdate_mutex);
                if (lock_ret!=0  ) {
                        sprintf(msg_buf, "HTXMP LIB ERROR : hxfupdate: pthread_mutex_unlock failed under START = %d \n",lock_ret);
                        hxfmsg(global_mp_htx_ds, rc, HTX_HE_HARD_ERROR, msg_buf);
                }

	}

	else{
		sprintf(msg_buf, "HTXMP LIB ERROR : hxfupdate: wrong type passed to hxfupdate call :type = %c",type);
		hxfmsg(global_mp_htx_ds, rc, HTX_HE_HARD_ERROR, msg_buf);
	}
    
   return(rc);
}


/*---------------------------------------------------------------------
 *  Thread safe "hxfpat" function protected by a lock for threads
 *----------------------------------------------------------------------*/
int 
hxfpat(char *filename, char *pattern_buf, int num_chars) { 

   int rc;

   pthread_once (&hxfpat_onceblock, hxfpat_once_init);
   pthread_mutex_lock (&hxfpat_mutex);
   rc=hxfpat_tefficient ((filename), (pattern_buf), (num_chars));
   pthread_mutex_unlock (&hxfpat_mutex);
   return(rc);
}


/*---------------------------------------------------------------------
 *  Thread safe "hxfsbuf" function protected by a lock for threads
 *----------------------------------------------------------------------*/

int 
hxfsbuf(char *buf, size_t len, char *fname, struct htx_data * ps) {    
 
   int rc;

   pthread_once (&hxfsbuf_onceblock, hxfsbuf_once_init); 
   pthread_mutex_lock (&hxfsbuf_mutex); 
   rc=hxfsbuf_tefficient (buf, len, fname, ps); 
   pthread_mutex_unlock (&hxfsbuf_mutex); 
   return(rc);
}

/*---------------------------------------------------------------------
 *  Thread safe "hxfcbuf" function protected by a lock for threads
 *----------------------------------------------------------------------*/

int 
hxfcbuf(struct htx_data * ps, char * wbuf, char * rbuf, size_t len, char * msg) {    
 
   int rc; 

   pthread_once (&hxfcbuf_onceblock, hxfcbuf_once_init); 
   pthread_mutex_lock (&hxfcbuf_mutex); 
   rc=hxfcbuf_calling_hxfsbuf_tsafe (ps, wbuf, rbuf, len, msg); 
   pthread_mutex_unlock (&hxfcbuf_mutex); 
   return(rc);
}

#ifndef __HTX_LINUX__
/*---------------------------------------------------------------------
 *  New function introduced for thread programming
 *  This function returns states of CPUS in the machine and how many are
 *  active
 *----------------------------------------------------------------------*/

int 
hxfinqcpu_state(int * cpustate, int * count_active_cpus, int * arch) { 


  int cpu_num, i;

  *cpustate=0; *count_active_cpus=0;
  *arch = _system_configuration.architecture;
  if (*arch != POWER_PC)
     return(-1);

  for (i=0; i < _system_configuration.ncpus ;i++){
                cpu_num = 1<<(31-i);
                *cpustate=*cpustate| cpu_num;
                (*count_active_cpus)++;
        }

  return(0);

} /* end hxfinqcpu */

/*---------------------------------------------------------------------
*  New function introduced for thread programming
*  This function binds the first available cpu from cpustate to the    
*  given thread in tid. That cpu bit is turned off.
*----------------------------------------------------------------------*/

int 
hxfbindto_a_cpu(int * cpustate, int  tid, cpu_t * cpunum) { 

  int check_arch, rc;
  int tmpstate;
  cpu_t tmpcpu;

  pthread_once (&hxfbindto_a_cpu_onceblock, hxfbindto_a_cpu_once_init);
  pthread_mutex_lock (&hxfbindto_a_cpu_mutex);

  check_arch = _system_configuration.architecture;
  if (check_arch != POWER_PC)
     return(-1);

#ifdef DEBUG1
  sprintf (msg, "Enter bind tid=%x, to cpustate=%x\n",tid, *cpustate);
  hxfmsg(statsp,0,7,msg);
#endif
  if (!(*cpustate))
     return (-2);

  tmpcpu=bitindex(cpustate);
  tmpstate=*cpustate&(~(1<<(31-(tmpcpu))));

#ifdef DEBUG1
  sprintf (msg,"Before bindprocessor tid=%x, to tmpcpu=%d, tmpstate=%x\n",tid, tmpcpu, tmpstate);
  hxfmsg(statsp,0,7,msg);
#endif
  rc=bindprocessor(BINDTHREAD,tid,tmpcpu);
  if (!rc) {
     *cpunum=tmpcpu;
     *cpustate=tmpstate;
     }
#ifdef DEBUG1
  sprintf (msg, "return tid=%x, to cpustate=%8x cpu=%d\n",tid, *cpustate, *cpunum);
  hxfmsg(statsp,0,7,msg);
#endif

  pthread_mutex_unlock (&hxfbindto_a_cpu_mutex);
  return(rc);
}

/*---------------------------------------------------------------------
*  New function introduced for thread programming
*  This function binds the first available cpu from cpustate to the    
*  given thread in tid. That cpu bit is turned off.
*----------------------------------------------------------------------*/

int 
hxfbindto_the_cpu(int cpu, int tid) { 

  int check_arch, rc;

  check_arch = _system_configuration.architecture;
  if (check_arch != POWER_PC)
     return(-1);
 
  rc=(bindprocessor(BINDTHREAD,tid,(cpu_t)cpu));
}

#endif
/*#endif*/

