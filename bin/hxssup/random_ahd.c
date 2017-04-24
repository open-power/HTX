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

/*  @(#)26       1.2.4.1  src/htx/usr/lpp/htx/bin/hxssup/random_ahd.c, htx_sup, htxubuntu 12/16/14 03:58:56  */

/*
 *   FUNCTIONS: rand_ahd 
 *              rand_sig_end
 *	        SIGALARM_handler
 *
 */

/*****************************************************************************/
/*                                                                           */
/* MODULE NAME    =    random_ahd.c                                          */
/* COMPONENT NAME =    hxssup (supervisor)                                   */
/* LPP NAME       =    HTX                                                   */
/*                                                                           */
/* DESCRIPTIVE NAME =  random activate and halt device  Exerciser            */
/*                                                                           */
/* STATUS =            Release 1 Version 0                                   */
/*                                                                           */
/* FUNCTION =          Randomly activate and halt device exercisers.         */
/*                                                                           */
/*                                                                           */
/*****************************************************************************/

#include <unistd.h>

#include "hxssup.h"

#ifdef  __HTX_LINUX__
#define SIGMAX  (SIGRTMAX)
#endif

int alrm_signal;

void rand_sig_end();

void SIGALARM_handler();

/*****************************************************************************/
/***************    rand_ahd()   *********************************************/
/*****************************************************************************/
/*                                                                           */

void rand_ahd()
{
  /*
   ***  variable definitions  *************************************************
   */
  char		      workstr[512];

  extern char         *program_name;   /* this program's name (argv[0])     */
  
  extern int          semhe_id;        /* semaphore id                      */

  extern int 	      rand_halt_hi;    /* upper bound for the range of random halt time   */
 
  extern int	      rand_halt_low;  /*  lower bound for the range of random halt time  */

  extern union shm_pointers shm_addr;  /* shared memory union pointers      */

  int                 rand_act_hi;     /* upper bound for the range of random activate time  */
  int                 rand_act_low;    /* Lower bound for the range of random activate time  */
  int                 errno_save;      /* errno save area. */
  int                 i, r, s;               /* loop counter     */
  int                 num_entries = 0; /* local number of shm HE entries    */
 
  static char	      process_name[] = "random_ahd";

  struct htxshm_HE    *p_htxshm_HE;    /* pointer to a htxshm_HE struct     */

  struct semid_ds     sembuffer;       /* semaphore buffer                  */

  struct sigaction    sigvector, sigvector2;       /* structure for signals */

  union shm_pointers  shm_addr_wk;     /* work ptr into shm                 */

  extern union semun semctl_arg;

  /*
   ***  beginning of executable code  *****************************************
   */
  program_name = process_name;  /* To make random_ahd appear as program name                                       in message.  */

  (void) send_message("random_ahd process started.", 0, HTX_SYS_INFO, HTX_SYS_MSG);

  /*
   ***  Set default SIGNAL handling  ******************************************
   */
  sigemptyset(&(sigvector.sa_mask));  /* do not block signals         */
  sigvector.sa_flags = 0;         /* do not restart system calls on sigs */
  sigvector.sa_handler = SIG_DFL;

  sigemptyset(&(sigvector2.sa_mask));  /* do not block signals         */
  sigvector2.sa_flags = 0;         /* do not restart system calls on sigs */
  sigvector2.sa_handler = SIG_DFL;

  for (i = 1; i <= SIGMAX; i++)
    (void) sigaction(i, &sigvector, (struct sigaction *) NULL);

  sigaddset(&(sigvector.sa_mask), SIGINT);
  sigaddset(&(sigvector.sa_mask), SIGQUIT);
  sigaddset(&(sigvector.sa_mask), SIGTERM);

  for (i = 1; i <= SIGMAX; i++)
    (void) sigaction(i, &sigvector2, (struct sigaction *) NULL);
   
  sigvector.sa_handler = (void (*)(int)) rand_sig_end;
  (void) sigaction(SIGINT, &sigvector, (struct sigaction *) NULL);
  (void) sigaction(SIGQUIT, &sigvector, (struct sigaction *) NULL);
  (void) sigaction(SIGTERM, &sigvector, (struct sigaction *) NULL);

  sigvector2.sa_handler = (void (*)(int)) SIGALARM_handler;
  (void) sigaction(SIGALRM, &sigvector2, (struct sigaction *) NULL);

  /*
   ***  set up shared memory addressing  *************************************
   */
  shm_addr_wk.hdr_addr = shm_addr.hdr_addr;  /* copy addr to work space  */
  (shm_addr_wk.hdr_addr)++;         /* skip over header                  */

  /*
   ***   Loop to change the each HE except mem to toggle between activate/halt           status 
   */

  while(TRUE)
  {
    p_htxshm_HE = shm_addr_wk.HE_addr;
    num_entries = (shm_addr.hdr_addr)->max_entries;

/*************************generate random halt time **********************/

    r = rand() % (rand_halt_hi - rand_halt_low + 1) + rand_halt_low;
    sleep(r);
    (void) sprintf(workstr, "Going to halt the exercisers.");
    (void) send_message(workstr, 0, HTX_SYS_INFO, HTX_SYS_MSG);


    for(i=0; i<num_entries; i++, p_htxshm_HE++)
    {
      /* determine HE status *************************************/
       
      if ((strncmp(p_htxshm_HE->sdev_id, "mem", 3) != 0) &&  /* not mem  */
          (strncmp(p_htxshm_HE->sdev_id, "tlbie", 5) != 0) &&   /* not tlbie  */
	  (p_htxshm_HE->PID != 0) &&     /* Not Dead?  */
          (p_htxshm_HE->tm_last_upd != 0) &&     /* HE started?  */
          ((p_htxshm_HE->max_cycles == 0) || ((p_htxshm_HE->max_cycles != 0) && (p_htxshm_HE->cycles < p_htxshm_HE->max_cycles)))  &&  /* not completed  */
      (semctl(semhe_id, 0, GETVAL, &sembuffer) == 0) &&   /* system active?   */
          (semctl(semhe_id, ((i * SEM_PER_EXER) + 6), GETVAL, &sembuffer) == 0) &&   /* not halted  */
          (semctl(semhe_id, ((i * SEM_PER_EXER) + 6 + 1), GETVAL, &sembuffer) == 0)) /* Not errored out  */
      {
            semctl_arg.val = 1; 
            semctl(semhe_id, ((i * SEM_PER_EXER) + 6), SETVAL, semctl_arg); 
      	    p_htxshm_HE->rand_halt = 1;
      }   /* end if  */
   }    /* end for   */
    alrm_signal = 0;
    alarm(300);  
    p_htxshm_HE = shm_addr_wk.HE_addr;

 /*********** check exercisers have actually stopped on semaphore *******/ 

  for ( i =0; i<num_entries; i++, p_htxshm_HE++)
    {
       if ( p_htxshm_HE->rand_halt == 1  ) { 
	while ((alrm_signal == 0) && (p_htxshm_HE->PID != 0) && ((p_htxshm_HE->max_cycles == 0) || ((p_htxshm_HE->max_cycles != 0) && (p_htxshm_HE->cycles < p_htxshm_HE->max_cycles))) && (semctl(semhe_id, ((i * SEM_PER_EXER) + 6), GETZCNT, &sembuffer) != 1));
       }
    }
    alarm((unsigned) 0);

/***************** generate random activate time    **********************/

   rand_act_hi = rand_halt_hi / 10;
   rand_act_low = rand_halt_low / 10;
   s = rand() % (rand_act_hi - rand_act_low + 1) + rand_act_low ;
   sleep(s);
   p_htxshm_HE = shm_addr_wk.HE_addr;
   for(i=0; i<num_entries; i++, p_htxshm_HE++)
    {
      /* determine HE status *************************************/
      if ((p_htxshm_HE->rand_halt == 1) &&   /*  halted by this process only  */
         (strncmp(p_htxshm_HE->sdev_id, "mem", 3) != 0) &&  /* not mem  */
         (strncmp(p_htxshm_HE->sdev_id, "tlbie", 5) != 0) &&   /* not tlbie  */
	 (p_htxshm_HE->PID != 0) &&     /* Not Dead?  */
         (p_htxshm_HE->tm_last_upd != 0) &&     /* HE started?  */
         (semctl(semhe_id, 0, GETVAL, &sembuffer) == 0) &&   /* system active?   */
          (semctl(semhe_id, ((i * SEM_PER_EXER) + 6), GETVAL, &sembuffer) == 1))    /* HE not active  */
      {
	    semctl_arg.val = 0;
            semctl(semhe_id, ((i * SEM_PER_EXER) + 6), SETVAL, semctl_arg);
            p_htxshm_HE->rand_halt = 0;
      }   /* end if  */
   }    /* end for   */
 
 } 
}
   
 
/*
 * NAME: rand_sig_end()
 *
 * FUNCTION: Handles SIGTERM, SIGQUIT, and SIGINT signals.
 *
 */

void rand_sig_end(int signal_number, int code, struct sigcontext *scp)
     /*
      * signal_number -- the number of the received signal
      * code -- unused
      * scp -- pointer to context structure
      */

{
    char workstr[512];

    (void) sprintf(workstr, "Received signal %d.  Exiting...", signal_number);
    (void) send_message(workstr, 0, HTX_SYS_INFO, HTX_SYS_MSG);
    exit(0);
    return;
} /* rand_sig_end() */

/*
 * NAME: SIGALARM_handler()
 *
 * FUNCTION: Handles SIGALARM signals.
 *
 */

void SIGALARM_handler(int signal_number, int code, struct sigcontext *scp)
     /*
      * signal_number -- the number of the received signal
      * code -- unused
      * scp -- pointer to context structure
      */

{
    char workstr[512];
    extern int alrm_signal;

    alrm_signal = 1;
    (void) sprintf(workstr, "Warning: Not all HE's stopped within allowed time.\nWill activate all the devices.");
    (void) send_message(workstr, 0, HTX_SYS_INFO, HTX_SYS_MSG);
    return;

} /*end of SIGALARM_handler  */

