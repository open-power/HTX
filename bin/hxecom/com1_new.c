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

/* @(#)88	1.5.1.2  src/htx/usr/lpp/htx/bin/hxecom/com1.c, exer_com, htx53A 5/24/04 17:26:13 */

/*
 *   COMPONENT_NAME: exer_com
 *
 *   FUNCTIONS: DefineGLOBALSems
 *		DefineMySems
 *		DefineShm
 *		Detach_Private
 *		GetSockID
 *		IncDeadCnt
 *		MasterInit
 *		SaveSemServID
 *		SaveSockID
 *		StartMasterInit
 *		StopMasterInit
 */

#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <sys/time.h>
#include <sys/sem.h>
#include <sys/types.h>

#ifdef __HTX_LINUX__
#include <sys/stat.h>
#endif

#include <stdlib.h>
#include <errno.h>
#include <signal.h>
#include <termios.h>
#include <sys/shm.h>
#include "hxecomdef.h"

#define GLOBAL_SEMID		shm_HXECOM->MasterSemid
#define BACKOUT_SEMID		shm_Private->BackoutSemid

extern   struct htx_data htx_ds;
extern char global_htx_home_dir[256];
extern char global_htx_log_dir[256];	

static void DefineGLOBALSems(int ExerNo);
static void MasterInit(char * hostname);
static void StartMasterInit(char *hostname);
static void StopMasterInit(void);


int DefineShm(char * hostname)
{
   /***************************************************************************/
   /* We are relying on the following behaviors to make this work:            */
   /*  - Shared memory is automatically initialized to 0 when created.        */
   /*  - shmat and shmget are atomic operations.                              */
   /*  - Between runs all process are killed. Therefore the 'nattch' field    */
   /*    is zero on startup.                                                  */
   /*  - No hxecom process will attach to shared memory until it is known     */
   /*    that the memory is valid.  Therefore if the 'nattch' count is > 0    */
   /*    it is safe to assume shared memory is a valid stable structure.      */
   /*  - Global semaphore referenced by BACKOUT_SEM_KEY is created by the     */
   /*    master process only after attaching to shared memory.                */
   /*  - This backout semaphore is never deleted prior to deleting shared     */
   /*    memory.  Therefore if the shared memory remains from a previous run, */
   /*    the backout semaphore remains and is at the proper initialization    */
   /*    value.                                                               */
   /*  - All checking is done on SHM_HXECOM_PRIV_KEY.  This is used by        */
   /*    hxecom parent processes only.  All other processes attach to         */
   /*    SHM_HXECOM_KEY.                                                      */
   /***************************************************************************/
   int mID, mIDp, i, rc;
   struct shmid_ds ShmDs;
   int ExerNo;
   int backoutSemId = -1;
   static RepairFlag = 0;
   pid_t PID;
   struct htx_data * stats = &htx_ds;
   char  msg_text[1024];

   PID = getpid();

   while(1) {
		/* check to see if BACKOUT_SEM_KEY exists now. Use flag=0 so that it
		   will not be created if it does not exist, Should return -1 if 
		   BACKOUT_SEM_KEY does not already existing */
      backoutSemId = semget(BACKOUT_SEM_KEY, 1, 0);
      errno = 0;
      if((mIDp = shmget(SHM_HXECOM_PRIV_KEY, sizeof(struct shm_Private_t), 
                        IPC_CREAT | S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | 
                                                     S_IROTH | S_IWOTH)) == -1) {
         sprintf(msg_text, "DefineShm: Error getting shared memory (shm_Private_t) - %s\n", STRERROR(errno));
         hxfmsg(stats, HTXERROR(EX_SHMGET4,errno), HTX_HE_SOFT_ERROR, msg_text);
         HE_exit(EX_SHMGET4);
      }
      if((mID = shmget(SHM_HXECOM_KEY, sizeof(struct shm_hxecom_t), 
                        IPC_CREAT | S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | 
                                                     S_IROTH | S_IWOTH)) == -1) {
         sprintf(msg_text, "DefineShm: Error getting shared memory (shm_hxecom_t) - %s\n", STRERROR(errno));
         hxfmsg(stats, HTXERROR(EX_SHMGET3,errno), HTX_HE_SOFT_ERROR, msg_text);
         HE_exit(EX_SHMGET3);
      }
      if(shmctl(mIDp, IPC_STAT, &ShmDs) == -1) {
         sprintf(msg_text, "DefineShm: Error getting status of shared memory - %s\n", STRERROR(errno));
         hxfmsg(stats, HTXERROR(EX_SHMCTL,errno), HTX_HE_HARD_ERROR, msg_text);
         HE_exit(EX_SHMCTL);
      }
      if(PID != (pid_t)ShmDs.shm_cpid && (int)ShmDs.shm_nattch > 0 && RepairFlag == 0) {
         /*********************************************************************/
         /* Since at least one process is attached to shared memory, it is    */
         /* valid by definition. If we own memory, we must initialize.        */
         /*********************************************************************/
         break;      
      }
      if(PID != (pid_t)ShmDs.shm_cpid) {
         /*********************************************************************/
         /* If we are here, shared memory may still be valid.  The master     */
         /* may have just not attached yet.  So we will do a stricter test.   */
         /*********************************************************************/
      
         if(backoutSemId == -1) {
            /******************************************************************/
            /* We have established that the backout sem didn't exist at the   */
            /* time the 'nattch' count was 0.  This implies that the old      */
            /* memory was deleted -- this is new memory and it is valid.      */
            /******************************************************************/
            break;
         }
         /*********************************************************************/
         /* The first process to execute DownSem(backoutSemId, 0) will        */
         /* cleanup old structures. After attaching to shared memory, it will */
         /* wakeup processes when it initailizes the backout semaphore.       */
         /*********************************************************************/
         DownSem(backoutSemId, 0, stats);
         if((mIDp = shmget(SHM_HXECOM_PRIV_KEY, sizeof(int), 0)) == -1) {
            sprintf(msg_text, "DefineShm: Error getting ID of private shared memory - %s\n", STRERROR(errno));
            hxfmsg(stats, HTXERROR(EX_SHMGET5,errno), HTX_HE_SOFT_ERROR, msg_text);
            HE_exit(EX_SHMGET5);
         }
         if((mID = shmget(SHM_HXECOM_KEY, sizeof(struct shm_hxecom_t), 0)) == -1) {
            sprintf(msg_text, "DefineShm: Error getting ID of old/new shared memory - %s\n", STRERROR(errno));
            hxfmsg(stats, HTXERROR(EX_BACK5,errno), HTX_HE_HARD_ERROR, msg_text);
            HE_exit(EX_BACK5);
         }
         if(shmctl(mIDp, IPC_STAT, &ShmDs) == -1) {
            sprintf(msg_text, "DefineShm: Error getting status of old/new hxecom-only shared memory - %s\n", 
                           STRERROR(errno));
            hxfmsg(stats, HTXERROR(EX_BACK6,errno), HTX_HE_HARD_ERROR, msg_text);
            HE_exit(EX_BACK6);
         }
         if((int)ShmDs.shm_nattch > 0) {
            /******************************************************************/
            /* We woke up after old shared memory has been replaced.    */
            /******************************************************************/
            UpSem(backoutSemId, 0, stats);
            break;
         }
         /*********************************************************************/
         /* Since no one can attach to shared memory, it is safe to remove.   */
         /* If we exit during cleanup, rest of exercisers hang.  Doesn't      */
         /* matter a whole lot.                                               */
         /*********************************************************************/
         RepairFlag = 1;
         sprintf(msg_text, "DefineShm: Old run of hxecom not cleaned Up\nAttempting cleanup.  If processes don't run, \nshutdown HTX and remove all \"hxecom\" processes\n");
         hxfmsg(stats, HTXERROR(EX_BACK4,0), HTX_HE_INFO, msg_text);
         if((shm_HXECOM = (struct shm_hxecom_t *)shmat(mID, (char *)0, 0)) == (struct shm_hxecom_t *) -1) {
            sprintf(msg_text, "DefineShm: Error attaching shared memory - %s\n", STRERROR(errno));
            hxfmsg(stats, HTXERROR(EX_SHMAT8,errno), HTX_HE_HARD_ERROR, msg_text);
            HE_exit(EX_SHMAT8);
         }
		 /* kill all hxecom procs except this one, and remove all shared mem and 			semaphones */
		 sprintf(msg_text, "%s/runcleanup/hxecom.runcleanup %d", global_htx_home_dir, PID);
		 system(msg_text);
        /*********************************************************************/
         /* Old shared memory has been deleted.  Start over -- we may not be  */
         /* the master process when we start over.                            */
         /*********************************************************************/
         RepairFlag = 0;
         continue;
      }
      else {  
         /*********************************************************************/
         /* (PID == ShmDs.shm_cpid) We created shared memory, Ok to attach.   */
         /*********************************************************************/

         if((shm_HXECOM = (struct shm_hxecom_t *)shmat(mID, (char *)0, 0)) == (struct shm_hxecom_t *) -1) {
            sprintf(msg_text, "DefineShm: Error attaching shared memory - %s\n", STRERROR(errno));
            hxfmsg(stats, HTXERROR(EX_SHMAT4,errno), HTX_HE_HARD_ERROR, msg_text);
            HE_exit(EX_SHMAT4);
         }
         if((shm_Private = (struct shm_Private_t *)shmat(mIDp, (char *)0, 0)) == (struct shm_Private_t *) -1) {
            sprintf(msg_text, "DefineShm: Error attaching private shared memory - %s\n", STRERROR(errno));
            hxfmsg(stats, HTXERROR(EX_SHMAT5,errno), HTX_HE_HARD_ERROR, msg_text);
            HE_exit(EX_SHMAT5);
         }
         ExerNo = 0;
         /*********************************************************************/
         /* DefineGLOBALSems will initialize backup sem to 1.  This will      */
         /* wakeup any processes sleeping on this semaphore.                  */
         /*********************************************************************/
         DefineGLOBALSems(ExerNo);
         /*********************************************************************/
         /* Lets put'm to sleep.                                              */
         /*********************************************************************/
         shm_HXECOM->global_sem_ready = 1;
         /* We created shared memory.  Need to create semaphore for access. */
         shm_HXECOM->mID = mID;
         shm_Private->mID = mIDp;
         shm_HXECOM->NoExer = 1;
         shm_HXECOM->exer[ExerNo].EXER_PID = PID;
         shm_pHXECOM->ExerNo = ExerNo;

         MasterInit(hostname);
         /* Let's wake them up.                                             */
         UpSem(GLOBAL_SEMID, SLEEP_SEM, stats);
         /* Now initialize own structure.                                   */
         return ExerNo;
      }
   }
   /***************************************************************************/
   /* All but one master process eventually come to this point.  They all     */
   /* wait here until the master process initializes all structures.          */
   /***************************************************************************/
   if((shm_Private = (struct shm_Private_t *)shmat(mIDp, (char *)0, 0)) == (struct shm_Private_t *) -1) {
      sprintf(msg_text, "DefineShm: Error attaching private shared memory - %s\n", STRERROR(errno));
      hxfmsg(stats, HTXERROR(EX_SHMAT6,errno), HTX_HE_SOFT_ERROR, msg_text);
      HE_exit(EX_SHMAT6);
   }
   if((shm_HXECOM = (struct shm_hxecom_t *)shmat(mID, (char *)0, 0)) == (struct shm_hxecom_t *) -1) {
      sprintf(msg_text, "DefineShm: Error attaching shared memory - %s\n", STRERROR(errno));
      hxfmsg(stats, HTXERROR(EX_SHMAT7,errno), HTX_HE_SOFT_ERROR, msg_text);
      HE_exit(EX_SHMAT7);
   }
   /* We didn't create. Need to wait until SLEEP_SEM is created. */
   /* We will only do busy waiting until the Master gets his act */
   /* together.                                                  */

   while(shm_HXECOM->global_sem_ready == 0)
      sleep(5);
   DownSem(GLOBAL_SEMID, SLEEP_SEM, stats);
   ExerNo = shm_HXECOM->NoExer++;
   if(ExerNo > EXER_MAX) {
      UpSem(GLOBAL_SEMID, SLEEP_SEM, stats);
      sprintf(msg_text, "DefineShm: Maximum exercisers/host exceeded. - EXER_MAX=%d\n", EXER_MAX);
      hxfmsg(stats, HTXERROR(EX_EXERNO,0), HTX_HE_SOFT_ERROR, msg_text);
      HE_exit(EX_EXERNO);
   }
   shm_HXECOM->exer[ExerNo].EXER_PID = PID;
   shm_pHXECOM->ExerNo = ExerNo;
   UpSem(GLOBAL_SEMID, SLEEP_SEM, stats);
   return ExerNo;
}



static void DefineGLOBALSems(int ExerNo)
{

    char  msg_text[1024];
	struct htx_data * stats = &htx_ds;
   if(ExerNo != 0) {
      sprintf(msg_text, "DefineGLOBALSems: Error \"ExerNo\" is invalid\n");
      hxfmsg(stats, HTXERROR(EX_DGERR,0), HTX_HE_SOFT_ERROR, msg_text);
#ifdef __DEBUG__
      printf(" exerno is not zero call HE_exit\n");
#endif
      HE_exit(EX_DGERR);
   }

   if((GLOBAL_SEMID = semget((MASTER_HXECOM_KEY), MASTER_NO_SEMS, 
                     IPC_CREAT | S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH)) == -1) { 
      sprintf(msg_text, "DefineGLOBALSems: Error creating semaphore - %s\n", STRERROR(errno));
      hxfmsg(stats, HTXERROR(EX_SEMCREAT1,errno), HTX_HE_SOFT_ERROR, msg_text);
#ifdef __DEBUG__
      printf("couldnot get semaphore\n");
#endif
      HE_exit(EX_SEMCREAT1);
   }
    /* We want everyone to go to sleep on this semaphore.           */
   SetSem(GLOBAL_SEMID, SLEEP_SEM, 0, stats);
    /* We just want serial access to the htxtty.doc file.           */
   SetSem(GLOBAL_SEMID, FILE_SEM,  1, stats);
    /* We just want serial access to the coord stats.               */
   SetSem(GLOBAL_SEMID, COORD_SEM, 1, stats);
    /* We just want serial access to the NoWriters.                 */
   SetSem(GLOBAL_SEMID, WRITE_SEM, 1, stats);
    /* We just want serial access while forking coordinators.       */
   SetSem(GLOBAL_SEMID, FORK_SEM, 1, stats);
   if((BACKOUT_SEMID = semget((BACKOUT_SEM_KEY), 1, 
                     IPC_CREAT | S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH)) == -1) { 
      sprintf(msg_text, "DefineGLOBALSems: Error creating backout semaphore - %s\n", STRERROR(errno));
      hxfmsg(stats, HTXERROR(EX_SEMCREAT3,errno), HTX_HE_SOFT_ERROR, msg_text);
      HE_exit(EX_SEMCREAT3);
   }
   /* We just want serial access in the backout code.              */
   SetSem(BACKOUT_SEMID, 0,  1, stats);

}



int DefineMySems(int ExerNo)
{
   int my_SemId;
   int i;
	char msg[100], msg_text[1024];
	struct htx_data * stats = &htx_ds;

   if(ExerNo < 0 || ExerNo >= shm_HXECOM->NoExer) {
      (void) sprintf(msg_text, "DefineMySems: \"ExerNo\" is invalid\n");
      (void) hxfmsg(stats, HTXERROR(EX_COM1,0), HTX_HE_SOFT_ERROR, msg_text);
      HE_exit(EX_COM1);
   }

   if((my_SemId = semget(MY_HXECOM_KEY+ExerNo, MY_NO_SEMS, 
                     IPC_CREAT | S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH)) == -1) { 
      (void) sprintf(msg_text, "DefineMySems: Error creating semaphore - %s\n", STRERROR(errno));
      (void) hxfmsg(stats, HTXERROR(EX_SEMCREAT2,errno), HTX_HE_SOFT_ERROR, msg_text);
      HE_exit(EX_SEMCREAT2);
   }

		sprintf(msg,"echo %x >> %shxecom_sids ", MY_HXECOM_KEY+ExerNo, global_htx_log_dir);
		system(msg);

   /* Start in state with write sleeping.                                     */
   for(i=WRITE_AHEAD0; i<MAXREMOTE; i++)
      SetSem(my_SemId, WRITE_AHEAD0 + i,  0, stats);
   return my_SemId;
}



static void StopMasterInit(void)
{
}



static void StartMasterInit(char *hostname)
{
   FILE * config_des;
   char temp_string[300];
/*   char * fullname, name[10]=LSTAT_FILE;   
#ifdef __HTX_LINUX__
   if((fullname = uname_s(name))==NULL){
      printf("error in getting LSTAT_FILE and uname\n");
      exit(0);
   }
#else
   fullname = name;
#endif
*/

   sprintf(temp_string, "%s/%s", global_htx_log_dir, LSTAT_FILE);
   config_des = fopen(temp_string, "a");
   fclose(config_des);

   sprintf(temp_string, "%s/%s", global_htx_log_dir, CONFIG_FILE);
   config_des = fopen(temp_string, "w");
   fprintf(config_des, "#Below are the remote addresses of machines\n");
   fprintf(config_des, "#tested with this run.  The address are from the\n");
   fprintf(config_des, "#rule COMNET_NAME.  This file can be used with\n");
   fprintf(config_des, "#shell scripts to recover remote test information.\n"); 
   fclose(config_des);

#ifdef DEBUG
      {
/*   while(debug_flag == 0)
      sleep(5);
*/
   }
#endif
}



static void MasterInit(char * hostname)
{
   StartMasterInit(hostname);

   /***************************************************************************/
   /* Insert here any actions that MUST BE performed only once.  These will   */
   /* not be executed for each process.  They will only be executed by the    */
   /* first process to execute DefineShm().                                   */
   /***************************************************************************/

   StopMasterInit();
}



struct id_t * SaveSockID(struct id_t ID)
{
   bcopy((char *) &ID, (char *) &shm_HXECOM->exer[shm_pHXECOM->ExerNo].ServerID, sizeof(ID));
   return &shm_HXECOM->exer[shm_pHXECOM->ExerNo].ServerID;
}


void GetSockID(struct id_t * ID)
{
    bcopy((char *) &shm_HXECOM->exer[shm_pHXECOM->ExerNo].ServerID, (char *) ID, sizeof(struct id_t));
}



void SaveSemServID(struct id_t SemServID)
{
   bcopy((char *) &SemServID, (char *) &shm_pHXECOM->SemServID, sizeof(SemServID));
}



void IncDeadCnt(void)
{
	struct htx_data * stats = &htx_ds;
   DownSem(GLOBAL_SEMID, SLEEP_SEM, stats);
   shm_HXECOM->DeadCnt++;
   shm_HXECOM->exer[shm_pHXECOM->ExerNo].dead_flag = 1;
   UpSem(GLOBAL_SEMID, SLEEP_SEM, stats);
}


void Detach_Private(struct shm_Private_t * ptr)
{
    int MID;
    struct shmid_ds ShmDs;
    int backoutSemId;
    struct semid_ds arg;

    if(ptr == (struct shm_Private_t *)NULL)
        return;

    MID = ptr->mID;
    backoutSemId = BACKOUT_SEMID;
    shmdt((char *)ptr);

    shmctl(MID, IPC_STAT, &ShmDs);
    if(ShmDs.shm_nattch == 0) {
        if(shmctl(MID, IPC_RMID, 0) != -1)
            semctl(backoutSemId, 0, IPC_RMID, arg);
    }
}


/*
char * uname_s(char * name)
{

  struct utsname sysinfo;

  if (uname( &sysinfo)== -1){
     printf("uname error\n");
     return NULL;
  }
  printf("sysname: %s\n nodename: %s\n release : %s\n version: %s\n machine: %s\n 
\n", sysinfo.sysname, sysinfo.nodename, sysinfo.release, sysinfo.version, sysinfo.
machine);
  return(strcat(name,sysinfo.nodename));
}*/

