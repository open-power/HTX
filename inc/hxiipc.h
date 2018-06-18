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
/* @(#)47  1.34.4.8  src/htx/usr/lpp/htx/inc/hxiipc.h, htx_libhtx, htxfedora 12/16/14 03:58:17 */
/* Component = htx_libhtx_;x */

#ifndef __HTX_INC_HXIIPC_H__
#define __HTX_INC_HXIIPC_H__


/*
 *  hxiipc.h -- HTX IPC (Inter-Process Communication) Include File
 */


/*  Include System IPC Header Files  **************************************/

#ifdef HTX_REL_tu320
#include <sys/hft.h>
#endif
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/sem.h>
#include <sys/shm.h>


#include <hxihtx.h> 
#if !defined(__HTX_LINUX__) && !defined(__OS400__)
#include <mesg.h>
#endif

#define RULE_NAME_LENGTH_LIMIT 100
#define CPU_UTLIZATION_LENGTH 128

#define SEM_PER_EXER 3
#define SEM_GLOBAL 6
#define SEM_POSITION_SYSCFG 1

/* ** Check for the MSGX in the msg.h file found in /usr/include/sys ** */
/* ** Also check if the msg.h file is the same as that found in AIX  ** */



#if defined(__HTX_LINUX__) || defined(__OS400__)
#define	MSGX	time_t	mtime;		/* time message was sent */	
#endif



/*  define msg_buf and msg_xbuf structures  *******************************/

struct htx_msg_buf {
        mtyp_t          mtype;            /* message type */
        struct htx_msg_data htx_data;     /* HTX msg data structure */
};

struct htx_msg_xbuf {
#ifndef	__HTX_LINUX__
	MSGX
#endif
       mtyp_t          mtype;            /* Message type */
        struct htx_msg_data htx_data;     /* HTX msg data structure */
};

/*  define message types  *************************************************/

#define HTX_SYS_MSG 50                    /* system status message        */
#define HTX_HE_MSG 100                    /* HE status message            */
#define HTX_SYS_FINAL_MSG 200             /* final HTX message -- kills
                                           *   message handler (hxsmsg)
                                           *   program.
                                           */


/*  IPC Key Definitions  **************************************************/

#define MSGLOGKEY  (key_t) 10101       /* Message Log msg Key             */
#define SEMHEKEY   (key_t) 10201       /* Hardware Exerciser sem Key      */
#define SEMOTKEY   (key_t) 10202       /* DRT Order Table QCB sem Key     */
#define SHMKEY     (key_t) 10301       /* shm Key                         */
#define SUPMSGKEY  (key_t) 10401       /* Supervisor Messaging Key        */
#define REMSHMKEY  (key_t) 10501

#define SHUTDOWN        1009

#define SCREEN_1        2001
#define SCREEN_2        2002
#define SCREEN_3        2003
#define SCREEN_4        2004
#define SCREEN_5        2005
#define SCREEN_6        2006
#define SCREEN_7        2007
#define SCREEN_8        2008
#define SCREEN_9        2009
#define SCREEN_9_A      2019
#define SCREEN_9_R      2039
#define SCREEN_9_R_D    2049
#define SCREEN_9_T      2059
#define SCREEN_9_T_D    2069

#define SCREEN_A        3000
#define MAXARRAY        1024
#define WEBCMDLINE      9020
#define CMDLINE         9030
#define FILE_TRANSFER   9021
#define FILEONHOST      9022
#define FILEONSUT       9023
#define FILEONSSM       9024




/*  Shared Memory Header typedef Structure Declaration  *******************/

struct htxshm_hdr {
        /* Flag integer - holds 32 one bit flags                             */
        unsigned       emc  : 1;          /* EMC Run Flag (on=EMC, off=REG)  */
        unsigned       started : 1;       /* System started flag (1=started) */
        unsigned       dst_beep : 1;      /* dst - beep on error flag (1=yes)*/
        unsigned       shutdown : 1;      /* Shutdown Flag (1=in progress)   */
        unsigned       hotplug_process_flag : 1;  /* hotplug process flag    */
        unsigned       sp27 : 1;          /* Spare Flag 27                   */
        unsigned       sp26 : 1;          /* Spare Flag 26                   */
        unsigned       sp25 : 1;          /* Spare Flag 25                   */
        unsigned       sp24 : 1;          /* Spare Flag 24                   */
        unsigned       sp23 : 1;          /* Spare Flag 23                   */
        unsigned       sp22 : 1;          /* Spare Flag 22                   */
        unsigned       sp21 : 1;          /* Spare Flag 21                   */
        unsigned       sp20 : 1;          /* Spare Flag 20                   */
        unsigned       sp19 : 1;          /* Spare Flag 19                   */
        unsigned       sp18 : 1;          /* Spare Flag 18                   */
        unsigned       sp17 : 1;          /* Spare Flag 17                   */
        unsigned       sp16 : 1;          /* Spare Flag 16                   */
        unsigned       sp15 : 1;          /* Spare Flag 15                   */
        unsigned       sp14 : 1;          /* Spare Flag 14                   */
        unsigned       sp13 : 1;          /* Spare Flag 13                   */
        unsigned       sp12 : 1;          /* Spare Flag 12                   */
        unsigned       sp11 : 1;          /* Spare Flag 11                   */
        unsigned       sp10 : 1;          /* Spare Flag 10                   */
        unsigned       sp9  : 1;          /* Spare Flag 9                    */
        unsigned       sp8  : 1;          /* Spare Flag 8                    */
        unsigned       sp7  : 1;          /* Spare Flag 7                    */
        unsigned       sp6  : 1;          /* Spare Flag 6                    */
        unsigned       sp5  : 1;          /* Spare Flag 5                    */
        unsigned       sp4  : 1;          /* Spare Flag 4                    */
        unsigned       sp3  : 1;          /* Spare Flag 3                    */
        unsigned       sp2  : 1;          /* Spare Flag 2                    */
        unsigned       sp1  : 1;          /* Spare Flag 1                    */
        unsigned int    max_entries;       /* maximum # of entries in shm     */
        unsigned int  num_entries;       /* # of entries currently in shm   */
        unsigned short pseudo_entries;    /* # of pseudo entries cur. in shm */
        unsigned short pseudo_entry_0;    /* index to first pseudo entry     */

#ifdef HTX_REL_tu320
        struct         hft_devs {         /* hft devices...................  */
                int             fileid[HFNUMVTS];  /* Up to 4 hft devices    */
                int             hf_devid[HFNUMVTS]; /* allowed.              */
        } hft_devices[4];                    /* Each hft can have up to      */
                                             /* HFNUMVTS (32) virtual terms. */
                                             /* 1 mouse and 1 gio per hft.   */
                                             /* Mouse and gio devices will   */
                                             /* use the first hft fileid     */
                                             /* entry for the hft assigned.  */
#endif

};


/*  Shared Memory HE typedef Structure Declaration  ***********************/

struct htxshm_HE {
        char           HE_name[DEV_ID_MAX_LENGTH];       /* Hardware Exerciser Name         */
        char           adapt_desc[12];    /* Adapter Description             */
        char           device_desc[16];   /* Device Description              */
        char           emc_rules[RULE_NAME_LENGTH_LIMIT];    /* EMC Rules File Name             */
        char           reg_rules[RULE_NAME_LENGTH_LIMIT];    /* REG Rules File Name             */
        char           sdev_id[DEV_ID_MAX_LENGTH];       /* /dev/???? Description           */
	char           cpu_utilization[CPU_UTLIZATION_LENGTH];
        unsigned short dma_chan;          /* DMA Channel                     */
        unsigned short test_id;           /* Current rules file stanza       */
        unsigned short intrpt_lev;        /* Interrupt Level                 */
        unsigned short load_seq;          /* Load Sequence Number (1-1000)   */
        unsigned short max_run_tm;        /* Max Run Time (x<idle_time, secs)*/
        unsigned short port;              /* Port Number                     */
        unsigned short priority;          /* Priority (1-19)                 */
        unsigned short slot;              /* Slot Number                     */
        int            PID;               /* Process ID                      */
        unsigned long long   max_cycles;  /* Maximum Cycles                  */
        unsigned int   idle_time;         /* Idle (sleep) update time (secs) */
        int            halt_sev_level;    /* sev code level for a HALT       */
        unsigned       cont_on_err : 1;   /* Continue on Error Flag          */
        unsigned       err_ack : 1;       /* Operator ack'ed error (1 = yes) */
#ifdef HTX_REL_tu320
        unsigned       hft_num : 2;       /* hft number (0-3)                */
        unsigned       VT_num : 5;        /* Virtual Terminal number (0-31)  */
#endif
        unsigned       hung_flag : 1;     /* HE hung flag (1=yes)            */
        unsigned       start_halted : 1;  /* Start HE halted (1=yes)         */
        unsigned       restart_HE : 1;    /* Restart HE on SIGUSR2 (1=yes)   */
        unsigned       user_term : 1;     /* User rq'd termination (1=yes)   */
        unsigned       DR_term : 1;          /* Spare Flag 19                   */
        unsigned       is_child : 1;      /* Spare Flag 18                   */
        unsigned       dup_device : 1;    /* Spare Flag 17                   */
        unsigned       log_vpd : 1;       /* Detailed error log              */
        unsigned       hung_exer : 1;     /* hung exerciser(1=yes)           */
        unsigned       halt_flag : 1;     /* halt exerciser(1=yes)           */
        unsigned       rand_halt : 1;     /* randomly halt the exerciser (1=yes)                    */
        unsigned       equaliser_halt : 1;/* Spare Flag 12                   */
        unsigned       hotplug_cpu : 1;   /* flag to register cpu hotplug    */
        unsigned       hotplug_mem : 1;   /* flag to register mem hotplug    */
        unsigned       hotplug_io  : 1;   /* flag to register ip hotplug     */
        unsigned       upd_test_id  : 1;  /* updated test_id to 1            */
        unsigned       started_delay_flag : 1;  /*  started delay flag       */
        unsigned       test_run_period_expired  : 1; /* test run period expired flag */
        unsigned       equaliser_flag  : 1; /* Equaliser Flag                */
        unsigned       offline_cpu  : 1;  /* Offline CPU                     */
        unsigned       sp3  : 1;          /* Spare Flag 3                    */
        unsigned       sp2  : 1;          /* Spare Flag 2                    */
        unsigned       sp1  : 1;          /* Spare Flag 1                    */
        long long      tm_last_err;       /* Time of Last Error              */
        long long      tm_last_upd;       /* Time of Last Call to htx_update */
        long long      run_time;          /* exerciser total run time        */
	long long	test_started_time; /* test started timestamp         */
	long long	test_start_delay; /* test start delay duration in seconds */
	long long	test_run_period;  /* test run duration in seconds    */
        float          data_trf_rate1;/* write data transfer rate        */
	    float 	       data_trf_rate2;/* read data transfer rate	     */
	    long double    throughput;        /* Read instruction throughput     */
	    unsigned long long   bad_others;        /* Total Number of Other Bad Op's  */
        unsigned long long   bad_reads;         /* Total Number of Bad Reads       */
        unsigned long long   bad_writes;        /* Total Number of Bad Writes      */
        unsigned long long   bytes_read;       /* Total Number of Bytes Read      */
        unsigned long long   bytes_writ;       /* Total Number of Bytes Written   */
        unsigned long long   good_others;       /* Total Number of Other Good Op's */
        unsigned long long   good_reads;        /* Total Number of Good Reads      */
        unsigned long long   good_writes;       /* Total Number of Good Writes     */
        unsigned long long  total_num_instructions;   /* Number of instructions executed */
        unsigned long long	 no_of_errs;          /* Number of errors                */
        unsigned long long   cycles;            /* how many times we finished      */
        char           slot_port[20];     /* Slot and Port number            */
		pthread_rwlock_t HE_rwlock_hxfupdate;
};

/* shared memory union pointer  *******************************************/

union shm_pointers {
        struct htxshm_hdr *hdr_addr;     /* system shared memory address     */
        struct htxshm_HE  *HE_addr;      /* system shared memory address     */
	unsigned long long padding;      /* 32 - 64 bit alignment            */
};

#if defined(__HTX_LINUX__) || defined(__OS400__)

union semun {
                int val;
                struct semid_ds * buf ;
                ushort * array ;
            };
#endif



#endif  /* __HTX_INC_HXIIPC_H__ */
