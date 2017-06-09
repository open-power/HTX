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

#include <stdio.h>
#include <pthread.h>
#include <signal.h>

extern int errno;

#ifndef __HTX_LINUX__
#include <sys/systemcfg.h>
#include <sys/processor.h>
#include <sys/mode.h>
#endif

#include <time.h>
#include <unistd.h>
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

void hxfupdate_once_init(void)
{
	pthread_mutexattr_init(&mta);
	pthread_mutexattr_settype(&mta, PTHREAD_MUTEX_RECURSIVE);
	pthread_mutex_init(&hxfupdate_mutex, &mta);
}

void hxfpat_once_init(void)
{
  pthread_mutex_init(&hxfpat_mutex, DEFAULT_ATTR_PTR);
}

void hxfcbuf_once_init(void)
{
  pthread_mutex_init(&hxfcbuf_mutex, DEFAULT_ATTR_PTR);
}

void hxfsbuf_once_init(void)
{
  pthread_mutex_init(&hxfsbuf_mutex, DEFAULT_ATTR_PTR);
}
void hxfbindto_a_cpu_once_init(void)
{
  pthread_mutex_init(&hxfbindto_a_cpu_mutex, DEFAULT_ATTR_PTR);
}

/*---------------------------------------------------------------------
 *  Thread safe "hxfupdate" function protected by a lock for threads
 *----------------------------------------------------------------------*/

void unblock_stats_th(void) { 
	pthread_cond_broadcast(&start_thread_cond);
   
    pthread_mutex_unlock(&create_thread_mutex);
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
hxfupdate(char type, struct htx_data * data) {    

	int rc = 0; 
        char msg_buf[1024];
	int lockinit1, lock_ret, lock_ret_1;
	lockinit1 = lock_ret = lock_ret_1 = 0;
	char    msg_send[MSG_TEXT_SIZE];

	if(type == MESSAGE) {
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

