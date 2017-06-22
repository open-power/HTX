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

/* "@(#)92	1.29.6.15  src/htx/usr/lpp/htx/lib/htx64/hxfupdate.c, htx_libhtx, htxubuntu 1/12/16 05:27:10"; */

#define addw(msw, lsw, num) \
{ \
    lsw += num; \
    if (lsw > 999999999) /* split the addition over two words? */\
      {\
        msw++; \
        lsw -= 1000000000; \
      } /* endif */ \
}

/****************************************************************************/
/*****  I B M   I n t e r n a l   U s e   O n l y  **************************/
/****************************************************************************/
/*                                                                          */
/* MODULE NAME    =    hxfupdate.c                                          */
/* COMPONENT NAME =    hxfupdate (updates HE information in HTX system)     */
/* LPP NAME       =    HTX                                                  */
/*                                                                          */
/* DESCRIPTIVE NAME =  HTX Hardware Exerciser Update Function.              */
/*                                                                          */
/* COPYRIGHT =         (C) COPYRIGHT IBM CORPORATION 1988                   */
/*                     LICENSED MATERIAL - PROGRAM PROPERTY OF IBM          */
/*                                                                          */
/* STATUS =            Release 1 Version 0                                  */
/*                                                                          */
/* FUNCTION = Updates HE information in the HTX system.                     */
/*    hxfupdate = updates HE information in HTX system.                     */
/*    htx_start = establish connections to HTX system data.                 */
/*    htx_update = update HE information in HTX system data.               */
/*    htx_error = format and issue error messages, update error information.*/
/*    htx_finish = update end of cycle information.                         */
/*    sendp = place message on HTX message queue.                           */
/*                                                                          */
/* COMPILER OPTIONS =  -I/src/master/htx/common -g -Nn3000 -Nd4000 -Np1200  */
/*                     -Nt2000                                              */
/*                                                                          */
/* CHANGE ACTIVITY =                                                        */
/*    DATE    :LEVEL:PROGRAMMER:DESCRIPTION                                 */
/*    MMMMMMMMNMMMMMNMMMMMMMMMMNMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMM*/
/*    04/25/88:1.0  :J FRETER  :INITIAL RELEASE                             */
/*    09/06/88:2.0  :J BURGESS :Modify to conform to AES Programming Center */
/*            :     :          :"Software Development Process Guide"        */
/*    10/13/88:3.0  :T HOMER   :changed message header format               */
/*    12/14/88:3.1  :J BURGESS :Cleaned up message handling.                */
/*            :     :          :                                            */
/*            :     :          :                                            */
/*            :     :          :                                            */
/*            :     :          :                                            */
/*            :     :          :                                            */
/*            :     :          :                                            */
/*            :     :          :                                            */
/*            :     :          :                                            */
/*            :     :          :                                            */
/*                                                                          */
/****************************************************************************/

#include <htx_local.h>
#include <sys/types.h>
#include <unistd.h>
#include <time.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/signal.h>
#include <stdlib.h>
#include <sys/shm.h>
#include <hxiipc64.h>
#include <hxihtx64.h>
#include <hxiconv.h>
#include <htxlibdef.h>
#include <scr_info64.h>
#include <libgen.h>
#include "hxihtxmp_vars_new.h"
#include "htxlibdef.h"
#ifdef _DR_HTX_
#include <sys/dr.h> /* DR reconfig changes */
#include <sys/procfs.h>
#endif

/****************************************************************************/
/*  Global Variable Declarations  *******************************************/
/****************************************************************************/

struct  htxshm_hdr      *hft_p_shm_hdr = 0;       /* pointer to shm header.   */
/* struct  htxshm_hdr      *p_shm_hdr; */      /* pointer to shm header.   */
struct  htxshm_HE       *hft_p_shm_HE;            /* pointer to shm HE info.  */
/* struct  htxshm_HE       *p_shm_HE;  */          /* pointer to shm HE info.  */

struct htx_data					*hft_p_htx_data = 0; 	/* added for DR */
tecg_struct *ecg_info;
texer_list *exer_info;
int cur_ecg_pos = 0;

/*FILE *debugfd;*/
int pseudo_dev_count=0;
struct  htxshm_hdr      *tmp_p_shm_hdr = 0;
tmisc_shm *rem_shm_addr;
int rem_shm_id;

#define msqid        (data->msqid)
#define sem_id       (data->sem_id)
#define p_shm_hdr    (data->p_shm_hdr)
#define p_shm_HE     (data->p_shm_HE)


#include <errno.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>

/*void set_misc_htx_data(struct htx_data *);*/
struct htx_data             *misc_htx_data;

int hxfadd(char *server_ip, struct htxshm_HE add_HE, char *ecg)
{
  struct hostent *he;
  struct sockaddr_in their_addr;
  struct timeval tval;
  int sockfd, indx, numbytes, subcmd;
  short cmd;
  char str_msg[800];
  char *tmp_str, recv_data[80];;

  if ((he=gethostbyname(server_ip)) == NULL) {
    herror("gethostbyname");
    exit(1);
  }
  if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
    perror("socket");
    exit(1);
  }

   tval.tv_sec =  30L;
   tval.tv_usec = 0L;

  if (setsockopt(sockfd,SOL_SOCKET,SO_RCVTIMEO,&tval,sizeof(struct timeval)) ==
-1) {
    perror("setsockopt");
    exit(1);
  }

  their_addr.sin_family = AF_INET;
  their_addr.sin_port = htons(3491);
  their_addr.sin_addr = *((struct in_addr *)(he->h_addr));
  memset(&(their_addr.sin_zero), '\0', 8);

  if (connect(sockfd, (struct sockaddr *)&their_addr,
     sizeof(struct sockaddr)) == -1) {
     perror("connect");
     return -1;
  }
  cmd = (short)4000;
  subcmd = sizeof(struct htxshm_HE);
  indx = 0;
  sprintf(str_msg,"%4x %4x %4x %s", cmd,subcmd,indx,ecg);
  tmp_str = (char*)htx_malloc( (strlen(str_msg)+9));
  sprintf(tmp_str,"%8lx",strlen(str_msg));
  strcat(tmp_str,str_msg);
  tmp_str[strlen(str_msg)+8] = '\0';
  if ((numbytes=send(sockfd, tmp_str, strlen(tmp_str), 0)) == -1) {
    perror("send");
    return -1;
  }
  htx_free(tmp_str);
  if (receive_all(sockfd, recv_data) <=0) {
      return -1;
  }
  else if (strcmp(recv_data, "@SENDMORE") == 0) {
     if ((numbytes=send(sockfd, (struct htxshm_HE *)&add_HE, sizeof(struct htxshm_HE), 0)) == -1) {
       perror("send");
       return -1;
     }
  }

  return 0;
}


int receive_all(int sock, char *str_rcv)
{
int size_rcv,numbytes;
char str_size[10];

if ((numbytes=recv(sock,&str_size, 8, 0)) <= 0) {
    printf("Received %d Bytes. errno = %d Exiting \n",numbytes, errno);
    perror("recvfrom");
   if (errno == EAGAIN) {
      printf("\ntimeout:No data received from stx daemon.\n");
      exit(0);
   }
    return(numbytes);
}
if (numbytes < 8) {
   return 0;
}
    str_size[8] = '\0';
    sscanf(str_size,"%x",&size_rcv);
    memset(str_rcv,0,size_rcv);
    if ((numbytes=recv(sock,str_rcv, (size_rcv), 0)) == -1) {
       printf("Received -1 bytes. \n");
       perror("recvfrom");
      return(numbytes);
    }
    return numbytes;
}

/****************************************************************************/
/*****  h x f u p d a t e ( )  **********************************************/
/****************************************************************************/
/*                                                                          */
/* FUNCTION NAME =     hxfupdate()                                          */
/*                                                                          */
/* DESCRIPTIVE NAME =  Updates HE information in the HTX system.            */
/*                                                                          */
/* FUNCTION =          Recieve data via the htx_data structure, format that */
/*                     data, and save it in the appropriate HTX system area.*/
/*                                                                          */
/* INPUT =             call - the type of update call:                      */
/*                            S - Start Call.  Used establish links to the  */
/*                                HTX system data and control structures.   */
/*                                This call is only called once; usually,   */
/*                                near the beginning of the HE program.     */
/*                            U - Update Call.  Used periodically to update */
/*                                information in the HTX system data        */
/*                                structures.                               */
/*                            E - Error Call.  Used to update error         */
/*                                information and to pass messages to the   */
/*                                hxsmsg message handler program.           */
/*                            F - Finish Call.  Used to update HTX system   */
/*                                data at the end of a test cycle.          */
/*                     data - the htx_data data structure.  Contains a      */
/*                            variety of HE specific information.           */
/*                                                                          */
/* OUTPUT =            Updated HTX data and control structures, and messages*/
/*                     to the hxsmsg message handler program.               */
/*                                                                          */
/* NORMAL RETURN =     0 to indicate successful completion.                 */
/*                                                                          */
/* ERROR RETURNS =       1 invalid call variable (hxfupdate - non fatal).   */
/*                                                                          */
/*                      11 Unable to get HTX message queue (htx_start -     */
/*                         fatal).                                          */
/*                      12 Unable to get HTX shared memory (htx_start -     */
/*                         fatal).                                          */
/*                      13 Unable to attach HTX shared memory (htx_start -  */
/*                         fatal).                                          */
/*                      14 Unable to get HTX semaphore structure (htx_start */
/*                         fatal).                                          */
/*                      15 Unable to find htx_data->sdev_id in HTX shared   */
/*                         memory structure (htx_start - fatal).            */
/*                                                                          */
/*                      51 Unable to place message on the HTX message queue */
/*                         (sendp - non fatal).                             */
/*                                                                          */
/* EXTERNAL REFERENCES                                                      */
/*                                                                          */
/*    NONE.                                                                 */
/*                                                                          */
/****************************************************************************/

int hxfupdate_tunsafe(char call, struct htx_data *data)
     /* call -- call type: START, UPDATE, ERROR, or FINISH                  */
     /* data -- HTX Hardware Exerciser data structure                       */
{
  register int i;                         /* loop counter.            */

  char    disp_time[30], disp_time1[30];

  struct  tm      *asc_time, asc_time1;
  struct semid_ds sembuffer;

  int     rc = 0,rc1;                             /* return code.             */
  int     rel_pos;                        /* relative position in mdt */

  time_t     stop_time;
  time_t     temp_time;

  static  unsigned semnum, err_semnum;            /* cont-halt on err sem num */
  static  int     save_time;              /* cycle start time.        */
  int system_halt_status;

	char    msg_send[MSG_TEXT_SIZE];
	char stat_fname[ENV_VAR_SIZE], log_dir[ENV_VAR_SIZE];
	char *dev_id, *he_name;
	char *tmp_path_temp;

  union semun {
     int val;
     struct semid_ds *buf;
     unsigned short *array;
  } arg;


  static  struct  sembuf  exer_halt_sops[1] =
    {
          {
            /* HE wait for exer halt semaphore */
            (unsigned short) 0,  /* value will be update as exer semaphorei num */
            (short) 0,
            SEM_UNDO
            }
    };


  static  struct  sembuf  system_halt_sops[1] =
    {
          {
            /* HE wait for 0 */
            (unsigned short) 0,
            (short) 0,
            SEM_UNDO
            }
    };
  


  static  struct  sembuf  wait_err_sops[4] =
    {
      {
        /* Global wait for 0 */
        (unsigned short) 0,
        (short) 0,
        SEM_UNDO
        },
          {
            /* HE wait for 0 */
            (unsigned short) 0,
            (short) 0,
            SEM_UNDO
            },
              {
                /* Global wait on Error */
                (unsigned short) 2,
                (short) 0,
                SEM_UNDO
                },
                  {
                    /* HE wait on Error */
                    (unsigned short) 0,
                    (short) 0,
                    SEM_UNDO
                    }
    };

  /********************************************************************/
  /***  Beginning of executable code.  ********************************/
  /********************************************************************/

  /********************************************************************/
  /* If run type is not "REG" or "EMC" then return immediately with   */
  /* no further processing.  This allows the hardware exerciser       */
  /* writer to execute a stand alone test without requiring two       */
  /* versions of his code.                                            */
  /********************************************************************/

  /********************************************************************/
  /*  for debugging, write into this file                             */
  /********************************************************************/

  /*debugfd = fopen("/tmp/exerlog", "a");*/


  for (i = 0; (i < 4) && (data->run_type[i] != '\0'); i++)
    data->run_type[i] = toupper ((int) data->run_type[i]);
  call = toupper ((int) call);

/* for creating device specific directories for keeping device logs */
if ( call == 'S' ) {

    misc_htx_data = data;/*will be used by sysfg later*/
	tmp_path_temp = getenv("HTX_LOG_DIR");
	if (tmp_path_temp == NULL){
		strcpy(data->htx_log_dir, "/tmp/");
	}
	else{
		if ((strlen ((char*)strcpy(log_dir,tmp_path_temp)))<ENV_VAR_SIZE){
			strcpy(data->htx_log_dir,tmp_path_temp);
			printf("data->htx_log_dir %s",data->htx_log_dir);
		}
		else{
			fprintf(stderr, "%s\n", "directory path creation exceeded the allowed limit");
               		fflush(stderr);
               		exit(0);
       		 }
	}
    char *temp_string = getenv("HTX_DR_TEST");
    if (temp_string == NULL){
        data->htx_dr = 0;
    }else{
        data->htx_dr = atoi(temp_string);
        if(data->htx_dr != 1){
            data->htx_dr = 0;
        }
    }

	dev_id = basename(data->sdev_id);
	he_name = basename(data->HE_name);

	snprintf(stat_fname,ENV_VAR_SIZE,"mkdir -p %s/htx/%s/%s",data->htx_log_dir,he_name,dev_id);
	rc1 = system(stat_fname);
	if(rc1 != 0)
		printf("[%d][%s]Failed to execute system call with errno=%d\n",__LINE__,__FUNCTION__,errno);
		
	snprintf(data->htx_exer_log_dir,ENV_VAR_SIZE,"%s/htx/%s/%s/",data->htx_log_dir,he_name,dev_id);

	int ret = setenv("HTX_EXER_LOG_DIR",data->htx_exer_log_dir,1);
	if(ret){
       	        fprintf(stderr, "%s\n", "environment var setting failed");
                fflush(stderr);
	}
}


  if ((strcmp (data->run_type, "REG") != 0) && (strcmp (data->run_type, "EMC") != 0) && (call != 'M') && (call != 'E')) {
	if ( call == 'S' ) {
		fprintf(stderr, "%s\n", IBM_copyright_string);
		fflush(stderr);
		/*set_misc_htx_data(data);*/
	}
    return (0);
  }


  switch (call)
    {
    case START:               /*  Startup Call  ***************************/
      sem_id = -1;
      p_shm_hdr = 0;
      hft_p_shm_hdr = 0;
      if ( (rel_pos = htx_start(data, &sem_id, &save_time)) > 0 )
        {
          /****************************************************/
          /* Set semaphore numbers for this hardware          */
          /* exerciser.  Semaphores are in groups of 2 for    */
          /* each hardware exerciser with a group of 6 global */
          /* semaphores preceding the first group of hardware */
          /* exerciser semaphores.                            */
          /****************************************************/

          exer_halt_sops[0].sem_num = (unsigned short) (((rel_pos - 1) * SEM_PER_EXER) + SEM_GLOBAL);
          wait_err_sops[1].sem_num = exer_halt_sops[0].sem_num;
          semnum = exer_halt_sops[0].sem_num;
          err_semnum = exer_halt_sops[0].sem_num + 1;
          wait_err_sops[3].sem_num = (unsigned short) err_semnum;

          rc = 0;                 /* set good return code.    */
        }
      else /* problem in htx_start()           */
        {
          rc = -1*rel_pos;        /* set bad return code.     */
          exit(rc);               /* RIP                      */
        } /* endif */
		/*set_misc_htx_data(data);*/
      break;
	case UPDATE:               /*  Update Call  ****************************/
		htx_update(data);
		htx_get_msg(data, msg_send);
		if (p_shm_HE->idle_time > (unsigned int) 0)
			(void) sleep(p_shm_HE->idle_time);
		rc = 0;
		break;

    case ERROR:               /*  Error Call  *****************************/
      if ((exer_halt_sops[0].sem_num > (unsigned int) 0)) 
        p_shm_HE->err_ack = 0;

		htx_update(data);  
		htx_get_msg(data, msg_send);
		htx_error(data,msg_send);

		if (!((strcmp (data->run_type, "REG") != 0) && (strcmp (data->run_type, "EMC") != 0))) {
			if (p_shm_HE->err_ack == 0) {
				rem_shm_addr->sock_hdr_addr->bcast_done = FALSE;
				rem_shm_addr->sock_hdr_addr->error_count++;
				/*sprintf(cmmd,"echo count = %d: dev= %s>>/tmp/hell",rem_shm_addr->sock_hdr_addr->error_count,p_shm_HE->sdev_id);
				/system(cmmd);*/
				strcpy(rem_shm_addr->sock_hdr_addr->system_status,"PARTIALLY RUNNING");
				strcpy(rem_shm_addr->sock_hdr_addr->error_dev,p_shm_HE->sdev_id);
				rem_shm_addr->sock_hdr_addr->last_error_time  = time((long *) 0);
				rem_shm_addr->sock_hdr_addr->last_update_time = time((long *) 0);
			}
		}
      break;

    case MESSAGE:               /*  Mesaage without an update ***************/
	    rc = htx_message(data,msg_send);
		break;

	case RECONFIG:
		p_shm_HE->DR_term = 1;
		break;

    case FINISH:               /*  Finish Call  ****************************/
      htx_update(data);
      rc = htx_finish(data, &save_time);
      if (p_shm_HE->max_cycles !=0) {
        if (p_shm_HE->cycles >= p_shm_HE->max_cycles) {
		  temp_time = p_shm_HE->tm_last_upd;	/* promote to long */
          asc_time = (struct tm *)htx_localtime_r(&temp_time, &asc_time1);
          (void) strcpy(disp_time, asctime_r(asc_time, disp_time1));
          disp_time[24] = '\0';
          data->error_code = 0;
          data->severity_code = (enum sev_code )7;
          (void) sprintf(data->msg_text,
                     "%-18s%-20s err=%-8.8x sev=%-1.1u %-14s\n\
Stopped - cycles run (%llu) = max_cycles (%llu).\n\n",
                     data->sdev_id,
                     &disp_time[4],
                     data->error_code,
                     data->severity_code,
                     data->HE_name,
                     p_shm_HE->cycles,
                     p_shm_HE->max_cycles);
          sendp(data, HTX_HE_MSG);
	  while ((p_shm_hdr)->shutdown == 0 && p_shm_HE->max_cycles) {
            sleep(1);
          } /* endwhile */
        } /* endif */
      } /* endif */
      break;
    default:                /*  Invalid Call  ***************************/
      rc = 1;
    } /* endswitch */

  if (exer_halt_sops[0].sem_num > (unsigned int) 0)   /* Successful htx_start()? */
    {
      if ((semctl(sem_id, semnum, GETVAL, &sembuffer) == 1) || ( (semctl(sem_id, err_semnum, GETVAL, &sembuffer) == 1) && ( p_shm_HE->cont_on_err == 0) ) )
        p_shm_HE->halt_flag = 1;
      else
        p_shm_HE->halt_flag = 0;

      if ( (data->severity_code <= p_shm_HE->halt_sev_level)
          && (call == ERROR) )
	   {
	      (p_shm_HE->no_of_errs)++;
          if (p_shm_HE->cont_on_err == 0)
        {

          if (p_shm_HE->test_id == 0 && p_shm_HE->upd_test_id == 0) {
            p_shm_HE->upd_test_id = 1;
          }

          stop_time = time((time_t *) 0);
          asc_time = (struct tm *)htx_localtime_r(&stop_time, &asc_time1);
          (void) strcpy(disp_time, asctime_r(asc_time, disp_time1));
          disp_time[24] = '\0';
          /*(void) sprintf(data->msg_text,
                         "%-18s%-20s err=%-8.8x sev=%-1.1u %-14s\n%-s\n",
                         data->sdev_id,
                         &disp_time[4],
                         0,
                         HTX_SYS_INFO,
                         data->HE_name,
                         "Hardware Exerciser program stopped on error.\n");*/
if(p_shm_HE->log_vpd==1)
  {
  (void) sprintf(data->msg_text,
        "\n---------------------------------------------------------------------\
        \nDevice id:%-18s\nTimestamp:%-20s\
        \nerr=%-8.8x\nsev=%-1.1d\nExerciser Name:%-14s\
        \nSerial No:%s\nPart No:%s\nLocation:%s\nFRU Number:%s\
        \nDevice:%s\nError Text:%-s\n\
        \n---------------------------------------------------------------------\n",
                 data->sdev_id,
                 &disp_time[4],
                 data->error_code,
                 data->severity_code,
                 data->HE_name,
                 data->serial_no,
                 data->part_no,
                 data->loc_code,
                 data->fru_no,
                 data->dev_desc,
                "Hardware Exerciser stopped on error\n");
   }
   else
   {
        (void) sprintf(data->msg_text,
                 "\n%-18s%-20s err=%-8.8x sev=%-1.1u %-14s\n%-s\n",
                 data->sdev_id,
                 &disp_time[4],
                 data->error_code,
                 data->severity_code,
                 data->HE_name,
                 "Hardware Exerciser stopped on an error\n");

    }
          (void) sendp(data, HTX_HE_MSG);

          arg.val = 1;
          (void) semctl(sem_id, err_semnum, SETVAL, arg);
	   while( ((p_shm_hdr)->shutdown == 0) && (semop(sem_id, wait_err_sops, 4) == -1) && (errno == EINTR) );
		 }
        } else {
	   while( ((p_shm_hdr)->shutdown == 0) && (semop(sem_id, exer_halt_sops, 1) == -1) && (errno == EINTR) );

	   system_halt_status = semctl(sem_id, 0, GETVAL, &sembuffer);
	   if(system_halt_status == 1) {
		if( semctl(sem_id, (exer_halt_sops[0].sem_num + 2), GETVAL, &sembuffer) != 1) {
			arg.val = 1;
			semctl(sem_id, (exer_halt_sops[0].sem_num + 2), SETVAL, arg);
		}
		while( ((p_shm_hdr)->shutdown == 0) && (semop(sem_id, system_halt_sops, 1) == -1) && (errno == EINTR) );
		if( semctl(sem_id, (exer_halt_sops[0].sem_num + 2), GETVAL, &sembuffer) != 0) {
			arg.val = 0;
			semctl(sem_id, (exer_halt_sops[0].sem_num + 2), SETVAL, arg);
		}
	   }
		
        } /* endif */
	  } /*endif */


  return (rc);

} /* hxfupdate() */



/****************************************************************************/
/*****  h t x _ s t a r t ( )  **********************************************/
/****************************************************************************/
/*                                                                          */
/* FUNCTION NAME =     htx_start()                                          */
/*                                                                          */
/* DESCRIPTIVE NAME =  Startup of hxfupdate function.                       */
/*                                                                          */
/* FUNCTION =          Establishes connections to the HTX data and control  */
/*                     structures, and initializes htx_data data structure. */
/*                                                                          */
/* INPUT =             data - the htx_data data structure.  Contains a      */
/*                            variety of HE specific information.           */
/*                     p_sem_id - pointer to an integer which will be used  */
/*                            to store the value of the HTX semaphore       */
/*                            structure id.                                 */
/*                     p_save_time - pointer to a integer which is used     */
/*                            to save the beginning of cycle time.          */
/*                                                                          */
/*                                                                          */
/* OUTPUT =            Connections to the HTX data and control structures,  */
/*                     initialized htx_data data structure and the beginning*/
/*                     of cycle time.                                       */
/*                                                                          */
/* NORMAL RETURN =     >= 0 to indicate the relative position of the HE     */
/*                        entry in the HTX shared memory structure.         */
/*                                                                          */
/* ERROR RETURNS =      11 Unable to get HTX message queue.                 */
/*                      12 Unable to get HTX shared memory.                 */
/*                      13 Unable to attach HTX shared memory.              */
/*                      14 Unable to get HTX semaphore structure.           */
/*                      15 Unable to find htx_data->sdev_id in HTX shared   */
/*                         memory structure.                                */
/*                                                                          */
/* EXTERNAL REFERENCES                                                      */
/*                                                                          */
/*    NONE.                                                                 */
/*                                                                          */
/****************************************************************************/

int htx_start(struct htx_data *data, int *p_sem_id, int *p_save_time)
{
  register int i;                 /* loop counter.                    */

  int     mem_id;                 /* HTX shared memory id.            */
  int     rc;                     /* return code.                     */
  int     MAX_EXER_ENTRIES=0;
  int     semkey,shmkey;
  /*texer_ipc *ex_ipc;*/
  char  tmp_sdev_id[DEV_ID_MAX_LENGTH];


  time_t  temp_time;
  time_t  current_time;
  time_t  started_time;
  struct  tm      *asc_time, asc_time1;
  char    disp_time[26], disp_time1[30];

    int lockinit1,lockinit1_1;
	lockinit1_1 = lockinit1 = 0;


  /********************************************************************/
  /***  Beginning of executable code.  ********************************/
  /********************************************************************/

  /********************************************************************/
  /* MSGLOGKEY is a key designated in HTX to represent messages sent  */
  /* to and received by the HTX message processing program.  This key */
  /* is used in the MSGGET system call to get the message queue       */
  /* identifier used for interprocess message switching.  See MSGGET  */
  /* system call in AIX Technical Reference Manual.                   */
  /********************************************************************/

  msqid = msgget(MSGLOGKEY, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP |
                 S_IROTH | S_IWOTH);

  if (msqid == -1)
    {
      data->error_code = errno;
      data->severity_code = HTX_SYS_HARD_ERROR;
      (void) sprintf(data->msg_text,
                     "%s for %s: Error in hxfupdate function.\n\
Unable to access HTX message queue.\nerrno = %d",
                     data->HE_name,
                     data->sdev_id,
                     data->error_code);

      if (sendp(data, HTX_SYS_MSG) != 0)        /* send msg to htx_error() */
        {
          perror(data->msg_text);
          (void) fflush(stderr);
        } /* endif */

      return(-11);
    } /* endif */

  /********************************************************************/
  /* SHMKEY is the key designated to represent shared memory          */
  /* throughout the HTX system.  This key is used in the SHMGET       */
  /* system call to get the shared memory region used for             */
  /* interprocess shared memory.  See SHMGET system call in AIX       */
  /* Technical Reference Manual for further details.                  */
  /********************************************************************/
/*********** REMSHM *********************/
      rem_shm_id = shmget(REMSHMKEY, 0, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH);
      if (rem_shm_id == -1) {
         data->error_code = errno;
         data->severity_code = HTX_SYS_HARD_ERROR;
         (void) sprintf(data->msg_text,
                     "%s for %s: Error in hxfupdate function.\n Unable to get HTX shared memory for rem_shm_id.\nerrno = %d\n",
                     data->HE_name,
                     data->sdev_id,
                     data->error_code);

         if (sendp(data, HTX_SYS_MSG) != 0) {      /* send message */
            perror(data->msg_text);
            (void) fflush(stderr);
         } /* endif */
         return(-12);
      } /* endif */

      rem_shm_addr = (tmisc_shm *)htx_malloc(sizeof(tmisc_shm));
      rem_shm_addr->sock_hdr_addr = (tsys_hdr  *) shmat(rem_shm_id, (char *) 0, 0);
      if ((int)(uintptr_t) rem_shm_addr->sock_hdr_addr == -1) {    /* problem? */
         data->error_code = errno;
         data->severity_code = HTX_SYS_HARD_ERROR;
         (void) sprintf(data->msg_text,
                     "%s for %s: Error in hxfupdate function.\n Unable to attach to HTX shared memory for rem_shm_addr.\nerrno = %d\n",
                     data->HE_name,
                     data->sdev_id,
                     data->error_code);

         if (sendp(data, HTX_SYS_MSG) != 0) {      /* send message */
            perror(data->msg_text);
            (void) fflush(stderr);
         } /* endif */
         return(-13);
      }       /* endif */

        /*fprintf(debugfd, " 64:going to get the shm:%d, size: %d\n",
		         SHMKEY, sizeof(tecg_struct ));
        fflush(debugfd);*/

      MAX_EXER_ENTRIES = rem_shm_addr->sock_hdr_addr->max_exer_entries;
      mem_id = shmget(SHMKEY, 0, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP |
                  S_IROTH | S_IWOTH);
      if (mem_id == -1) {
         data->error_code = errno;
         data->severity_code = HTX_SYS_HARD_ERROR;
         (void) sprintf(data->msg_text,
                     "%s for %s: Error in hxfupdate function.\n Unable to get HTX shared memory for exer_info .\nerrno = %d\n",
                     data->HE_name,
                     data->sdev_id,
                     data->error_code);

         if (sendp(data, HTX_SYS_MSG) != 0) {      /* send message */
            perror(data->msg_text);
            (void) fflush(stderr);
         } /* endif */
         return(-14);
      } /* endif */

      exer_info = (texer_list *) shmat(mem_id, (char *) 0, 0);
      if ((int)(uintptr_t) exer_info == -1) {    /* problem? */
           data->error_code = errno;
         data->severity_code = HTX_SYS_HARD_ERROR;
         (void) sprintf(data->msg_text,
                     "%s for %s: Error in hxfupdate function.\n Unable to attach to shared memory.\nerrno = %d\n",
                     data->HE_name,
                     data->sdev_id,
                     data->error_code);
           if (sendp(data, HTX_SYS_MSG) != 0) {        /* send msg to htx_error() */
            perror(data->msg_text);
            (void) fflush(stderr);
         } /* endif */
         return(-15);
      }       /* endif */
        /*fprintf(debugfd, " 64:going to get the ecg_info size: %d\n",
		         sizeof(texer_list ));
        fflush(debugfd);*/

     shmkey = -1;
	  semkey = -1;
     for (i = 0; i < MAX_EXER_ENTRIES; i++) {
        /*fprintf(debugfd, " 64:i=%d, exer_name:%s, dev_id: %s\n",
                          i, LIB_EXER_NAME(i), data->sdev_id );
        fflush(debugfd);*/
         if (dupdev_cmp( i,&data->sdev_id[5] )) {
            shmkey = LIB_ECGEXER_SHMKEY(i);
            semkey = LIB_ECGEXER_SEMKEY(i);
            break;
         }
     }

	  if ( shmkey == -1 ) {
         data->error_code = errno;
         data->severity_code = HTX_SYS_HARD_ERROR;
         (void) sprintf(data->msg_text,
                     "%s for %s: Error in hxfupdate function.\n smkey for exerciser is -1.\n",
                     data->HE_name,
                     data->sdev_id);

         if (sendp(data, HTX_SYS_MSG) != 0) {      /* send message */
            perror(data->msg_text);
            (void) fflush(stderr);
         } /* endif */
         return(-16);
	  }


/*********** REMSHM *********************/

  mem_id = shmget(shmkey, 0, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP |
                  S_IROTH | S_IWOTH);
  if (mem_id == -1)
    {
      data->error_code = errno;
      data->severity_code = HTX_SYS_HARD_ERROR;
      (void) sprintf(data->msg_text,
                     "%s for %s: Error in hxfupdate function.\n\
Unable to get HTX shared memory.\nerrno = %d",
                     data->HE_name,
                     data->sdev_id,
                     data->error_code);

      if (sendp(data, HTX_SYS_MSG) != 0)       /* send message */
        {
          perror(data->msg_text);
          (void) fflush(stderr);
        } /* endif */

      return(-17);
    } /* endif */

  /********************************************************************/
  /* The SHMAT system call attaches the shared memory segment         */
  /* associated with the shared memory identifier mem_id (from the    */
  /* SHMGET system call).  See SHMAT system call in AIX Technical     */
  /* Reference Manualfor further details.  The return value from      */
  /* SHMAT is cast to the structure type for hardware exerciser       */
  /* statistical accumulators and error messages.                     */
  /********************************************************************/

  p_shm_hdr = (void *) shmat(mem_id, (char *) 0, 0);
  if (p_shm_hdr == (struct htxshm_hdr *) -1)
    {
      data->error_code = errno;
      data->severity_code = HTX_SYS_HARD_ERROR;
      (void) sprintf(data->msg_text,
                     "%s for %s: Error in hxfupdate function.\n\
Unable to attach HTX shared memory.\nerrno = %d",
                     data->HE_name,
                     data->sdev_id,
                     data->error_code);

      if (sendp(data, HTX_SYS_MSG) != 0)       /* send message */
        {
          perror(data->msg_text);
          (void) fflush(stderr);
        } /* endif */

      return(-18);
    } /* endif */
  hft_p_shm_hdr = p_shm_hdr;
  data->mem_id = mem_id;

  /********************************************************************/
  /* SEMHEKEY is the key designated in HTX to represent hardware      */
  /* exerciser semaphores.  This key is used in the SEMGET system     */
  /* call to get the semaphore identifier used for interprocess       */
  /* semaphores.  See SEMGET system call in AIX Technical Reference   */
  /* Manualfor further details.                                       */
  /********************************************************************/

  *p_sem_id = semget(semkey, 0, 0);
  if (*p_sem_id == -1)
    {
      data->error_code = errno;
      data->severity_code = HTX_SYS_HARD_ERROR;
      (void) sprintf(data->msg_text,
                     "%s for %s: Error in hxfupdate function.\n\
Unable to get HTX semaphore structure.\nerrno = %d",
                     data->HE_name,
                     data->sdev_id,
                     data->error_code);

      if (sendp(data, HTX_SYS_MSG) != 0)       /* send message */
        {
          perror(data->msg_text);
          (void) fflush(stderr);
        } /* endif */

      return(-19);
    } /* endif */

  /********************************************************************/
  /* Read through the entries in the HTX system shared memory to find */
  /* a device id that equals the device id passed from the hardware   */
  /* exerciser.  This results in setting the shared memory pointer to */
  /* the array entry that contains the HTX information for this       */
  /* device.                                                          */
  /********************************************************************/


  p_shm_HE = (void *) (p_shm_hdr + 1);
  for (i = 0; i < p_shm_hdr->max_entries; i++)
    {
	htx_strcpy(tmp_sdev_id, p_shm_HE->sdev_id);

         if ((htx_strncmp(&data->sdev_id[5], strtok(tmp_sdev_id,"("),sizeof(data->sdev_id[5])) == 0) &&(p_shm_HE->PID == getpid()))
		       break;
               p_shm_HE++;

    } /* endfor */

  if (i == p_shm_hdr->max_entries)     /* Unable to find /dev/???? ? */
    {
      p_shm_HE = 0;

      data->error_code = 15;
      data->severity_code = HTX_HE_HARD_ERROR;
      (void) sprintf(data->msg_text,
                     "%s for %s: Error in hxfupdate function.\n\
Unable to find %s in HTX shared memory structure.\n",
                     data->HE_name,
                     data->sdev_id,
                     data->sdev_id);

      if (sendp(data, HTX_HE_MSG) != 0)       /* send message */
        {
          perror(data->msg_text);
          (void) fflush(stderr);
        } /* endif */

      return(-20);
    }
  else
    rc = i + 1;             /* return code = HE relative pos.   */

  hft_p_shm_HE = p_shm_HE;
  hft_p_htx_data = data;

  if(data->hotplug_cpu == 1) {
	p_shm_HE->hotplug_cpu = data->hotplug_cpu;
  } else {
	p_shm_HE->hotplug_cpu = 0;
  }

  if(data->hotplug_mem == 1) {
	p_shm_HE->hotplug_mem = data->hotplug_mem;
  } else {
	p_shm_HE->hotplug_mem = 0;
  }

  if(data->hotplug_io == 1) {
	p_shm_HE->hotplug_io = data->hotplug_io;
  } else {
	p_shm_HE->hotplug_io = 0;
  }

  /********************************************************************/
  /* Set all statistical accumulators in shared memory for the        */
  /* hardware exerciser to zero.  Also set accumulators in passed     */
  /* DATA structure to zero.                                          */
  /********************************************************************/

  p_shm_HE->cycles = 0;
  p_shm_HE->tm_last_err = 0;
  p_shm_HE->tm_last_upd = 0;
  p_shm_HE->run_time = 0;
  p_shm_HE->bad_others = 0;
  p_shm_HE->bad_reads = 0;
  p_shm_HE->bad_writes = 0;
  p_shm_HE->bytes_read1 = 0;
  p_shm_HE->bytes_read2 = 0;
  p_shm_HE->bytes_writ1 = 0;
  p_shm_HE->bytes_writ2 = 0;
  p_shm_HE->good_others = 0;
  p_shm_HE->good_reads = 0;
  p_shm_HE->good_writes = 0;
  p_shm_HE->total_num_instructions = 0;
  p_shm_HE->data_trf_rate1 = 0;
  p_shm_HE->data_trf_rate2 = 0;
  p_shm_HE->throughput = 0;
  p_shm_HE->test_id = 0;
  p_shm_HE->rand_halt = 0;
  p_shm_HE->upd_test_id = 0;
  data->bad_others = 0;
  data->bad_reads = 0;
  data->bad_writes = 0;
  data->bytes_read = 0;
  data->bytes_writ = 0;
  data->good_others = 0;
  data->good_reads = 0;
  data->good_writes = 0;
  data->num_instructions = 0;
  data->test_id = 0;
  data->miscompare_count = 0;
  strcpy(data->loc_code,"Not Available");
  strcpy(data->dev_desc,"Not Available");
  strcpy(data->serial_no,"Not Available");
  strcpy(data->fru_no,"Not Available");
  strcpy(data->part_no,"Not Available");

  p_shm_HE->tm_last_upd = time((time_t *) 0);
  *p_save_time = p_shm_HE->tm_last_upd;     /* for cycle time calc.     */

	/* to support started delay time */
	if(p_shm_HE->test_start_delay != 0) {
		started_time = current_time = time((time_t *) 0);
		data->error_code = 0;
		data->severity_code = (enum sev_code )7;
		temp_time = started_time;
		asc_time = (struct tm *)htx_localtime_r(&temp_time, &asc_time1);
		strcpy(disp_time, asctime_r(asc_time, disp_time1));
		disp_time[24] = '\0';

		(void) sprintf(data->msg_text,
			"\n---------------------------------------------------------------------\
			\nDevice id:%-18s\nTimestamp:%-20s\
			\nerr=%-8.8x\nsev=%-1.1d\nExerciser Name:%-14s\
			\nDevice:%s\
			\nMessage Text:test start delay is set as %llu seconds, going to sleep...\
			\n---------------------------------------------------------------------\n",
			data->sdev_id,
			&disp_time[4],
			data->error_code,
			data->severity_code,
			data->HE_name,
			data->dev_desc,
			p_shm_HE->test_start_delay);

		if (sendp(data, HTX_HE_MSG) != 0) {
			perror(data->msg_text);
			fflush(stderr);
		} /* endif */

		p_shm_HE->started_delay_flag = 1;
		while( (current_time < (started_time + p_shm_HE->test_start_delay) ) && ((p_shm_hdr)->shutdown == 0 ) ) {
			sleep(1);
			current_time = time((time_t *) 0);
		}
		p_shm_HE->started_delay_flag = 0;

		temp_time = current_time;
		asc_time = (struct tm *)htx_localtime_r(&temp_time, &asc_time1);
		strcpy(disp_time, asctime_r(asc_time, disp_time1));
		disp_time[24] = '\0';

		(void) sprintf(data->msg_text,
			"\n---------------------------------------------------------------------\
			\nDevice id:%-18s\nTimestamp:%-20s\
			\nerr=%-8.8x\nsev=%-1.1d\nExerciser Name:%-14s\
			\nDevice:%s\
			\nMessage Text:test start delay (%llu seconds) is completed, start running...\
			\n---------------------------------------------------------------------\n",
			data->sdev_id,
			&disp_time[4],
			data->error_code,
			data->severity_code,
			data->HE_name,
			data->dev_desc,
			p_shm_HE->test_start_delay);

		if (sendp(data, HTX_HE_MSG) != 0) {
			perror(data->msg_text);
			fflush(stderr);
		} /* endif */

	}
	p_shm_HE->test_started_time = time((time_t *) 0);


    lockinit1 = pthread_rwlockattr_init(&rwlattr_update);
    if (lockinit1!=0  ) {
	if ((lockinit1 == EAGAIN) || (lockinit1 == ENOMEM)){
            usleep(10);
            lockinit1_1=pthread_rwlockattr_init(&rwlattr_update);
            if(lockinit1_1 !=0){
                sprintf(data->msg_text,"rwlock attr initialisation failed with rc=%d\n", lockinit1);
          if (sendp(data, HTX_SYS_MSG) != 0)       /* send message */
        {
          perror(data->msg_text);
          (void) fflush(stderr);
        } /* endif */

            }
        }
        else{
        sprintf(data->msg_text,"rwlock attr initialisation failed with rc=%d\n", lockinit1);
          if (sendp(data, HTX_SYS_MSG) != 0)       /* send message */
        {
          perror(data->msg_text);
          (void) fflush(stderr);
        } /* endif */

        }
    }


    lockinit1 = pthread_rwlockattr_setpshared(&rwlattr_update, PTHREAD_PROCESS_SHARED);
    if (lockinit1!=0  ) {
    sprintf(data->msg_text,"sharing rwlock attr across processes failed with rc=%d\n", lockinit1);
          if (sendp(data, HTX_SYS_MSG) != 0)       /* send message */
        {
          perror(data->msg_text);
          (void) fflush(stderr);
        } /* endif */

    }
    lockinit1=pthread_rwlock_init(&(p_shm_HE->HE_rwlock_hxfupdate),&rwlattr_update);
    if (lockinit1!=0  ) {
        if ( lockinit1 == EBUSY ){
        sprintf(data->msg_text,"p_shm_HE->HE_rwlock_hxfupdate_init()1 failed with rc=%d\n", lockinit1);
      if (sendp(data, HTX_SYS_MSG) != 0)       /* send message */
        {
          perror(data->msg_text);
          (void) fflush(stderr);
        } /* endif */

        }
	else if ((lockinit1 == EAGAIN) || (lockinit1 == ENOMEM)){
            usleep(10);
        lockinit1_1=pthread_rwlock_init(&(p_shm_HE->HE_rwlock_hxfupdate),&rwlattr_update);
            if(lockinit1_1 !=0){
                    sprintf(data->msg_text,"p_shm_HE->HE_rwlock_hxfupdate_lock() failed in retry with rc=%d\n", lockinit1_1);
      if (sendp(data, HTX_SYS_MSG) != 0)       /* send message */
        {
          perror(data->msg_text);
          (void) fflush(stderr);
        } /* endif */

            }
        }
        else{
                sprintf(data->msg_text,"p_shm_HE->HE_rwlock_hxfupdate_lock() failed with rc=%d\n", lockinit1);
      if (sendp(data, HTX_SYS_MSG) != 0)       /* send message */
        {
          perror(data->msg_text);
          (void) fflush(stderr);
        } /* endif */

        }
    }


  return (rc);

} /* htx_start() */




/****************************************************************************/
/*****  h t x _ u p d a t e ( )  ********************************************/
/****************************************************************************/
/*                                                                          */
/* FUNCTION NAME =     htx_update()                                        */
/*                                                                          */
/* DESCRIPTIVE NAME =  Process HE specific HTX data.                        */
/*                                                                          */
/* FUNCTION =          Updates the HE specific HTX data for this program.   */
/*                                                                          */
/* INPUT =             data - the htx_data data structure.  Contains a      */
/*                            variety of HE specific information.           */
/*                                                                          */
/* OUTPUT =            Updated HE specific HTX data.                        */
/*                                                                          */
/* NORMAL RETURN =     0 to indicate successful completion.                 */
/*                                                                          */
/* ERROR RETURNS =     None.                                                */
/*                                                                          */
/* EXTERNAL REFERENCES                                                      */
/*                                                                          */
/*    NONE.                                                                 */
/*                                                                          */
/****************************************************************************/

int htx_update(struct htx_data *data)
{
	int rc1,rc2;
	rc1 = rc2 =0;
  int clock;
  /********************************************************************/
  /***  Beginning of executable code.  ********************************/
  /********************************************************************/

  if ((strcmp (data->run_type, "REG") == 0) ||
      (strcmp (data->run_type, "EMC") == 0))
    {
	rc1 = pthread_rwlock_wrlock(&(p_shm_HE->HE_rwlock_hxfupdate));
    if (rc1!=0  ) {
        if ( rc1 == EDEADLK ) {
				sprintf(data->msg_text,"lock inside htx_update failed with errno=%d for device id %s \n",rc1,data->sdev_id);
                return(sendp(data, HTX_HE_MSG));
        }
    }


	clock = time((time_t *) 0);
      if (p_shm_HE->halt_flag == 0)
		p_shm_HE->run_time += (clock - p_shm_HE->tm_last_upd);

	p_shm_HE->tm_last_upd = clock;

      /********************************************************************/
      /* Add to statistical accumulators in shared memory, then set       */
      /* accumulators in passed DATA structure to zero to prepare for     */
      /* next call to this function.                                      */
      /********************************************************************/

      p_shm_HE->bad_others += data->bad_others;
      p_shm_HE->bad_reads += data->bad_reads;
      p_shm_HE->bad_writes += data->bad_writes;
      addw(p_shm_HE->bytes_read1,p_shm_HE->bytes_read2,data->bytes_read);
      addw(p_shm_HE->bytes_writ1,p_shm_HE->bytes_writ2,data->bytes_writ);
      p_shm_HE->data_trf_rate1 = (((float)(p_shm_HE->bytes_writ1) / p_shm_HE->run_time) * 1000000000) + ((float)(p_shm_HE->bytes_writ2 ) / p_shm_HE->run_time);
      p_shm_HE->data_trf_rate2 = (((float)p_shm_HE->bytes_read1 / p_shm_HE->run_time) * 1000000000) + ((float)p_shm_HE->bytes_read2 / p_shm_HE->run_time);
      p_shm_HE->good_others += data->good_others;
      p_shm_HE->good_reads += data->good_reads;
      p_shm_HE->good_writes += data->good_writes;
      p_shm_HE->total_num_instructions += data->num_instructions;
      p_shm_HE->throughput = (double)p_shm_HE->total_num_instructions / p_shm_HE->run_time;
      p_shm_HE->test_id = data->test_id;
      if ( (p_shm_HE->upd_test_id == 0) && (p_shm_HE->test_id > 0 || p_shm_HE->cycles > 0 ) ) {
         p_shm_HE->upd_test_id = 1;
      } 
    rc2=pthread_rwlock_unlock(&(p_shm_HE->HE_rwlock_hxfupdate));
    if (rc2 !=0  ) {
			sprintf(data->msg_text,"unlock inside htx_update failed with errno=%d for device id %s \n",rc1,data->sdev_id);
            return(sendp(data, HTX_HE_MSG));
    }
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

  return 0;

} /* htx_update() */

int htx_get_msg(struct htx_data *data, char *msg_send)
{
  char    disp_time[30], disp_time1[30];

  time_t     call_time;
  static   int once_only_flag = 1;

  char htxstr[80];
  char temp[80];
  char *spstr, spstr1[80], spstr2[80],sp1[80],sp2[80],sp3[80];
  char *name, *tmp, *tmp1;
  FILE *fvp;
  char spare1[80], *name1, *tmps1, *tmps;

  struct  tm      *asc_time, asc_time1;


  /********************************************************************/
  /***  Beginning of executable code.  ********************************/
  /********************************************************************/

  /********************************************************************/
  /* The TIME system call returns the current time in seconds since   */
  /* 00:00:00 GMT, January 1, 1970.  The LOCALTIME subroutine         */
  /* converts this time to the current date and time adjusted for the */
  /* time zone and daylight savings.                                  */
  /********************************************************************/

  if (strlen(data->msg_text) > MAX_TEXT_MSG)       /* message too long? */
    data->msg_text[MAX_TEXT_MSG] = '\0';   /* truncate to max length */


  call_time = time((time_t *) 0);
  asc_time = (struct tm *)htx_localtime_r(&call_time, &asc_time1);
  (void) strcpy(disp_time, asctime_r(asc_time, disp_time1));
  disp_time[24] = '\0';

  if ((strcmp (data->run_type, "REG") != 0) && (strcmp (data->run_type, "EMC") != 0)) {
        (void) sprintf(msg_send,
                       "\n%-18s%-20s err=%-8.8x sev=%-1.1u %-14s\n%-s\n",
                                data->sdev_id,
                                &disp_time[4],
                                data->error_code,
                                data->severity_code,
                                data->HE_name,
                                data->msg_text);
  }
  else {
     if (p_shm_HE->log_vpd==1) {
        if(once_only_flag == 1){
                once_only_flag++;

                /*
                 * Increment this flag so that the code below
                 * is executed only once.
                 */

                 if(strstr(data->sdev_id,"rhdisk")!=NULL)
                 {
                         sprintf(htxstr,"lscfg -pvl %s",data->sdev_id+6);
                 }
                 else
                 {
                         sprintf(htxstr,"lscfg -pvl %s",data->sdev_id+5);
                 }

                 fvp=popen(htxstr,"r");

                if(fvp == NULL)
                {
                        printf("Error in opening pipe.");
                        exit(125);
                }
                 while(fgets(temp,80,fvp)!=NULL)
                {
		        if(strlen(temp) == 1) continue;
                        strcpy(spstr2,temp);
                        spstr = strpbrk(temp,"0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ");
                        if(spstr !=NULL)
                           strcpy(spstr1, spstr);
                        sscanf(spstr2,"%s %s %[^\n]",sp1,sp2,sp3);
                                if((strstr(spstr2,"mem"))!=NULL)
                                {
                                        while(fgets(temp,80,fvp)!=NULL)
                                        {
                                                strcpy(spare1,temp);
                                                name1=(char *)strtok(temp,":");
                                                if(name1!=NULL)
                                                {
                                                        tmps=(char*)strpbrk(spare1,":");
                                                        if(tmps != NULL )
                                                           tmps1=(char *)strpbrk(tmps,"0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ");
                                                        if(((strstr(name1,"Name"))!=NULL) && (tmps1 != NULL))
                                                        {
                                                                strcpy(data->dev_desc,tmps1);
                                                                data->dev_desc[strlen(data->dev_desc)]='\0';
                                                                printf("\n the device description is %s",data->dev_desc);
                                                                fflush(stdout);
                                                        }
                               	                        else if(((strstr(name1,"Physical Location"))!=NULL) && (tmps1 != NULL))
                                                        {
                                                        strcpy(data->loc_code,tmps1);
                                                        data->loc_code[strlen(data->loc_code)]='\0';
                                                        printf("\n the value of the location code is %s",data->loc_code);
                                                        fflush(stdout);
                                                        }
                                                }
                                                if(strstr(temp,":")==NULL)
                                                {
                                                        strcpy(spstr1,temp);
                                                        name=(char *)strtok(temp,".");
                                                        tmp=(char *)strpbrk(spstr1,"..");
                                                        if(tmp != NULL )
                                                           tmp1=(char *)strpbrk(tmp,"1234567890ABCDEFGHIJKLMNOPQRSTUVWXYZ");
                                                                if((strstr(name,"Serial Number")!=NULL) && (tmp1 != NULL))
                                                                {
                                                                        strcpy(data->serial_no,tmp1);
                                                                        data->serial_no[strlen(data->serial_no)]='\0';
                                                                        printf("\n the value of serial no is %s",data->serial_no);
                                                                        fflush(stdout);
                                                                }
                                                                else if((strstr(name,"Part Number")!=NULL) && (tmp1 != NULL))
                                                                {
                                                                        strcpy(data->part_no,tmp1);
                                                                        data->part_no[strlen(data->part_no)]='\0';
                                                                        printf("\n the value of the part number is %s",data->part_no);
                                                                        fflush(stdout);
                                                                }
                                                                else if((strstr(name,"FRU Number")!=NULL) && (tmp1 != NULL))
                                                                {
                                                                strcpy(data->fru_no,tmp1);
                                                                data->fru_no[strlen(data->fru_no)]='\0';
                                                                printf("\n the value of the fru_number is %s",data->fru_no);
                                                                fflush(stdout);
                                                                }
                                                }
                                        }
                                        break;
                                }
                                else if((strstr(spstr2,data->sdev_id+6)) != NULL)
                                {
                                        strcpy(data->loc_code,sp2);
                                        /*data->loc_code[strlen(data->loc_code)]='\0';*/
                                        strcpy(data->dev_desc,sp3);
                                        /*data->dev_desc[strlen(data->dev_desc)]='\0';*/
                                }

                                name=(char *)strtok(spstr,".");
                                tmp=(char *)strpbrk(spstr1,"..");
                                if(tmp != NULL)
                                    tmp1=(char *)strpbrk(tmp,"1234567890ABCDEFGHIJKLMNOPQRSTUVWXYZ");
				if(name != NULL)
				{
                                   if((strcmp(name,"Serial Number")==0) && (tmp1 != NULL))
                                   {
                                        strcpy(data->serial_no,tmp1);
                                        data->serial_no[strlen(data->serial_no)]='\0';
                                   }
                                   else if((strcmp(name,"Part Number")==0) && (tmp1 != NULL))
                                   {
                                          strcpy(data->part_no,tmp1);
                                          data->part_no[strlen(data->part_no)]='\0';
                                   }
                                   else if((strcmp(name,"FRU Number")==0) && (tmp1 != NULL))
                                   {
                                          strcpy(data->fru_no,tmp1);
                                          data->fru_no[strlen(data->fru_no)]='\0';
                                   }
				}
                        }/* End while fgets */
           pclose(fvp);
        }
     }
     else {
        (void) sprintf(msg_send,
                       "\n%-18s%-20s err=%-8.8x sev=%-1.1u %-14s\n%-s\n",
                                data->sdev_id,
                                &disp_time[4],
                                data->error_code,
                                data->severity_code,
                                data->HE_name,
                                data->msg_text);

    }
  }
	return(0);
}

int htx_message(struct htx_data *data, char* msg_send){

	/* htx_message for sending messages to the message queue for normal messages or error messages*/

    int rc1,rc2;
    rc1 = rc2 =0;


	size_t str_length;
	time_t call_time;
	struct  tm      *asc_time, asc_time1;
	char    disp_time[30], disp_time1[30];
	char    error_msg[256];
	static  struct  htx_msg_buf msgb;
	static  struct  htx_msg_buf *msgp = &msgb;
	
	call_time = time((time_t *) 0);
	asc_time = (struct tm *)htx_localtime_r(&call_time, &asc_time1);
	(void) strcpy(disp_time, asctime_r(asc_time, disp_time1));
	disp_time[24] = '\0';

	str_length = strlen(msg_send);
	if (msg_send[str_length - 2] != '\n'){
		(void) strcat(msg_send, "\n");
	}

  if ((strcmp (data->run_type, "REG") != 0) && (strcmp (data->run_type, "EMC") != 0)) {
        (void) sprintf(msg_send,
                       "\n%-18s%-20s err=%-8.8x sev=%-1.1u %-14s\n%-s\n",
                                data->sdev_id,
                                &disp_time[4],
                                data->error_code,
                                data->severity_code,
                                data->HE_name,
                                data->msg_text);
  }
  else {
     if (p_shm_HE->log_vpd==1) {

        (void) sprintf(msg_send,
                      "\n---------------------------------------------------------------------\
                       \nDevice id:%-18s\nTimestamp:%-20s\
                       \nerr=%-8.8x\nsev=%-1.1d\nExerciser Name:%-14s\
                       \nSerial No:%s\nPart No:%s\nLocation:%s\nFRU Number:%s\
                       \nDevice:%s\nError Text:%-s\n\
                       \n---------------------------------------------------------------------\n",
                                data->sdev_id,
                                &disp_time[4],
                                data->error_code,
                                data->severity_code,
                                data->HE_name,
                                data->serial_no,
                                data->part_no,
                                data->loc_code,
                                data->fru_no,
                                data->dev_desc,
                                data->msg_text);
     }
     else {
        (void) sprintf(msg_send,
                       "\n%-18s%-20s err=%-8.8x sev=%-1.1u %-14s\n%-s\n",
                                data->sdev_id,
                                &disp_time[4],
                                data->error_code,
                                data->severity_code,
                                data->HE_name,
                                data->msg_text);

    }
  }


	if ((msqid != -1) && ((strcmp (data->run_type, "EMC") == 0) ||(strcmp (data->run_type, "REG") == 0) ))
	{
		if (p_shm_HE > (struct htxshm_HE *) 0)
		{
			if (data->severity_code < HTX_SYS_INFO){

				rc1 = pthread_rwlock_wrlock(&(p_shm_HE->HE_rwlock_hxfupdate));
				if (rc1!=0  ) {
					if ( rc1 == EDEADLK ) {
							sprintf(data->msg_text,"lock inside htx_message failed with errno=%d for device id %s \n",rc1,data->sdev_id);
							return(sendp(data, HTX_HE_MSG));
					 }
				}


				p_shm_HE->tm_last_upd = call_time;
				p_shm_HE->tm_last_err = call_time;
    
				rc2=pthread_rwlock_unlock(&(p_shm_HE->HE_rwlock_hxfupdate));
				if (rc2 !=0  ) {
					sprintf(data->msg_text,"unlock inside htx_message failed with errno=%d for device id %s \n",rc1,data->sdev_id);
					return(sendp(data, HTX_HE_MSG));
			    }
			} /* endif */

			strcpy(msgp->htx_data.msg_text, msg_send);
			strcpy(msgp->htx_data.sdev_id, data->sdev_id);
			strcpy(msgp->htx_data.HE_name, data->HE_name);
			msgp->htx_data.severity_code = data->severity_code;
			msgp->htx_data.error_code = data->error_code;
			msgp->mtype = HTX_HE_MSG;

			if (msgsnd(msqid, msgp, (sizeof(msgb) - sizeof(mtyp_t)), 0) != 0)
			{
				(void) sprintf(error_msg,
				"%s for %s: Error in hxfupdate function.\n\
				Unable to send message.\nerrno = %d",
                     data->HE_name,
                     data->sdev_id, errno);
				perror(error_msg);
				(void) fflush(stderr);
				return(51);
			} /* endif */
		} /*endif of (p_shm_HE > (struct htxshm_HE *) 0)*/
    } /* endif of strcmp (data->run_type*/
	else
	{
		if(data->severity_code >= HTX_SYS_INFO){
			(void) fprintf(stdout, "%s", msg_send);
			fflush(stdout);
		} /*endif*/
	} /*end else */

  return(0);
} /*htx_message*/


/****************************************************************************/
/*****  h t x _ e r r o r ( )  **********************************************/
/****************************************************************************/
/*                                                                          */
/* FUNCTION NAME =     htx_error()                                          */
/*                                                                          */
/* DESCRIPTIVE NAME =  Process HE specific Error/Information data.          */
/*                                                                          */
/* FUNCTION =          Updates the HE specific HTX error data and sends     */
/*                     messages to the hxsmsg HTX message handler program.  */
/*                                                                          */
/* INPUT =             data - the htx_data data structure.  Contains a      */
/*                            variety of HE specific information.           */
/*                                                                          */
/* OUTPUT =            Updated HE specific HTX error data, and messages     */
/*                     queued for the hxsmsg HTX message handler program.   */
/*                                                                          */
/* NORMAL RETURN =     0 to indicate successful completion.                 */
/*                                                                          */
/* ERROR RETURNS =     -51 Unable to place message on the HTX message queue */
/*                         (sendp - non fatal).                             */
/*                                                                          */
/* EXTERNAL REFERENCES                                                      */
/*                                                                          */
/*    NONE.                                                                 */
/*                                                                          */
/****************************************************************************/


void htx_error(struct htx_data *data, char* msg_send){	

	char    *libhtx_HOE;
	int rc =0;
	size_t str_length;
	str_length = strlen(msg_send);
	if (msg_send[str_length - 2] != '\n')
		(void) strcat(msg_send, "\n");

	htx_message(data,msg_send);

	(void) fprintf(stderr, "%s", msg_send);
	fflush(stderr);
		           /*
           *  If this is running standalone, then look at the environment
           *  variable LIBHTX_HOE.  If it is set, but is null, kill yourself.
           *  Otherwise treat the returned string like an AIX command and use
           *  system() to run it.  Careful kids, don't try this at home.
           */
	libhtx_HOE = getenv("LIBHTX_HOE");
	if (( libhtx_HOE != (void *) NULL )&&(strcmp (data->run_type, "REG") != 0) && (strcmp (data->run_type, "EMC") != 0))
	{
		if ( *libhtx_HOE == 0 )
        {
			(void) kill(getpid(), SIGTERM);
		}
		else
		{
			rc = system(libhtx_HOE);
			if(rc != 0)
				printf("[%d][%s]Failed to execute system call with errno=%d\n",__LINE__,__FUNCTION__,errno);
		}
	}

  return;

} /* htx_error() */



/****************************************************************************/
/*****  h t x _ f i n i s h ( )  ********************************************/
/****************************************************************************/
/*                                                                          */
/* FUNCTION NAME =     htx_finish()                                         */
/*                                                                          */
/* DESCRIPTIVE NAME =  Finish processing HE specific end of cycle info.     */
/*                                                                          */
/* FUNCTION =          Updates the HE specific HTX data when an "End of     */
/*                     Cycle" has occurred.                                 */
/*                                                                          */
/* INPUT =             data - the htx_data data structure.  Contains a      */
/*                            variety of HE specific information.           */
/*                     p_save_time - pointer to the integer which           */
/*                            contains the start of cycle time value.       */
/*                                                                          */
/* OUTPUT =            Updated HE specific HTX data including a new start   */
/*                     of cycle time.                                       */
/*                                                                          */
/* NORMAL RETURN =     0 to indicate successful completion.                 */
/*                                                                          */
/* ERROR RETURNS =     -51 Unable to place message on the HTX message queue */
/*                         (sendp - non fatal).                             */
/*                                                                          */
/* EXTERNAL REFERENCES                                                      */
/*                                                                          */
/*    NONE.                                                                 */
/*                                                                          */
/****************************************************************************/

int htx_finish(struct htx_data *data, int *p_save_time)
{
  char    disp_time[26], disp_time1[30];

  struct  tm      *asc_time, asc_time1;

  time_t   temp_time;

  /********************************************************************/
  /***  Beginning of executable code.  ********************************/
  /********************************************************************/

  /********************************************************************/
  /* The TIME system call returns the current time in seconds since   */
  /* 00:00:00 GMT, January 1, 1970.  The LOCALTIME subroutine         */
  /* converts this time to the current date and time adjusted for the */
  /* time zone and daylight savings.  The ASCTIME subroutine converts */
  /* this time to a meaningful character string.  For further         */
  /* details, see TIME system call, LOCALTIME subroutine, and ASCTIME */
  /* subroutine in the AIX Technical Reference manual.                */
  /********************************************************************/

  p_shm_HE->cycles++;
  temp_time = p_shm_HE->tm_last_upd;	/* promote to long */
  asc_time = (struct tm *)htx_localtime_r(&temp_time, &asc_time1);
  (void) strcpy(disp_time, asctime_r(asc_time, disp_time1));
  disp_time[24] = '\0';

  data->error_code = 0;
  data->severity_code = (enum sev_code )7;

 if ((strcmp(data->run_type, "REG") != 0) && (strcmp(data->run_type, "EMC") != 0)) {         /* if NOT "REG" and "EMC" */
/*  if (strcmp(data->run_type, "REG") != 0)           */


      /*(void) sprintf(data->msg_text,
                     "%-18s%-20s err=%-8.8x sev=%-1.1u %-14s\n\
End of Hardware exerciser program cycle.\nCycle time = %d seconds.\n",
                     data->sdev_id,
                     &disp_time[4],
                     data->error_code,
                     data->severity_code,
                     data->HE_name,
                     (p_shm_HE->tm_last_upd - *p_save_time));*/
if(p_shm_HE->log_vpd==1)
  {
  (void) sprintf(data->msg_text,
       "\n---------------------------------------------------------------------\
         \nDevice id:%-18s\nTimestamp:%-20s\
        \nerr=%-8.8x\nsev=%-1.1d\nExerciser Name:%-14s\
        \nSerial No:%s\nPart No:%s\nLocation:%s\nFRU Number:%s\
        \nDevice:%s\
        \nMessage Text:End of Hardware Exerciser program.cycle time =%llu seconds.\
        \n---------------------------------------------------------------------\n",
                 data->sdev_id,
                 &disp_time[4],
                 data->error_code,
                 data->severity_code,
                 data->HE_name,
                 data->serial_no,
                 data->part_no,
                 data->loc_code,
                 data->fru_no,
                 data->dev_desc,
         (p_shm_HE->tm_last_upd - *p_save_time));
   }
   else
   {
        (void) sprintf(data->msg_text,
                 "\n%-18s%-20s err=%-8.8x sev=%-1.1u %-14s\n \
                \nMessage text:End of hardware exerciser program.cycle time=%llu\n",
                 data->sdev_id,
                 &disp_time[4],
                 data->error_code,
                 data->severity_code,
                 data->HE_name,
                 (p_shm_HE->tm_last_upd - *p_save_time));

    }


      *p_save_time = p_shm_HE->tm_last_upd;

      /********************************************************************/
      /* Specifying a message type of 150 to the sendp() function ensures */
      /* that this message is received only after all other outstanding   */
      /* HTX_ERR messages have been processed, but before the HTX         */
      /* termination message is received.  This is due to the message     */
      /* types assigned in the HTX system and to the way the message      */
      /* processing program receives messages.                            */
      /********************************************************************/

      return (sendp(data, HTX_HE_MSG));
    }
  return (0);

} /* htx_finish() */



/****************************************************************************/
/*****  s e n d p ( )  ******************************************************/
/****************************************************************************/
/*                                                                          */
/* FUNCTION NAME =     sendp()                                              */
/*                                                                          */
/* DESCRIPTIVE NAME =  Send a process message to the HTX message handler.   */
/*                                                                          */
/* FUNCTION =          Sends a HE message to the HTX message handler prog   */
/*                     (hxsmsg) via an ipc queueing structure.              */
/*                                                                          */
/* INPUT =             data - The htx_data data structure.  Contains a      */
/*                            variety of HE specific information.           */
/*                     item - A pointer to a char string which contains     */
/*                            the message.                                  */
/*                     msg_type - An arbitrary int value assigned to the    */
/*                            message.  If two messages with different      */
/*                            message type values are in the message queue  */
/*                            at the same time, the message with the lowest */
/*                            message type value will be serviced first.    */
/*                            The HTX system uses the following scheme:     */
/*                                msg_type = 100 for regular HE messages.   */
/*                                msg_type = 150 for End of Cycle messages. */
/*                                msg_type = 200 for System Shutdown msg.   */
/*                                                                          */
/*                                                                          */
/* OUTPUT =            Updated HE specific HTX error data, and messages     */
/*                     queued for the hxsmsg HTX message handler program.   */
/*                                                                          */
/* NORMAL RETURN =     0 to indicate successful completion.                 */
/*                                                                          */
/* ERROR RETURNS =     -51 Unable to place message on the HTX message queue */
/*                         (sendp - non fatal).                             */
/*                                                                          */
/* EXTERNAL REFERENCES                                                      */
/*                                                                          */
/*    NONE.                                                                 */
/*                                                                          */
/****************************************************************************/

int sendp(struct htx_data *data, mtyp_t msg_type)
{
  char    error_msg[256];

  static  struct  htx_msg_buf msgb;
  static  struct  htx_msg_buf *msgp = &msgb;


  /********************************************************************/
  /***  Beginning of executable code.  ********************************/
  /********************************************************************/

  strcpy(msgp->htx_data.msg_text, data->msg_text);
  strcpy(msgp->htx_data.sdev_id, data->sdev_id);
  strcpy(msgp->htx_data.HE_name, data->HE_name);
  msgp->htx_data.severity_code = data->severity_code;
  msgp->htx_data.error_code = data->error_code;

  msgp->mtype = msg_type;

  if (msgsnd(msqid, msgp, (sizeof(msgb) - sizeof(mtyp_t)), 0) != 0)
    {
      (void) sprintf(error_msg,
                     "%s for %s: Error in hxfupdate function.\n\
Unable to send message.\nerrno = %d",
                     data->HE_name,
                     data->sdev_id, errno);
      perror(error_msg);
      (void) fflush(stderr);
      return(51);
    } /* endif */

  return(0);

} /* sendp() */



int dupdev_cmp ( int index, char * my_exerid )
{

   char * tmp_ptr, tmp_str[48];

   if (LIB_EXER_NAME(index)[0] == '\0')
      return 0;


   strcpy(tmp_str, LIB_EXER_NAME(index));
   tmp_ptr = strtok(tmp_str, "(");
   if (tmp_ptr == NULL)
      return 0;
   /*if( strcmp(tmp_ptr, EXER_NAME(index)) == 0 )
     fprintf(debugfd,"%s:this is real exer\n",my_exerid);
	 fflush(debugfd);*/

   if ((strcmp(my_exerid, tmp_ptr) == 0) && (LIB_EXER_PID(index) == getpid())  ) {
      return 1;
   }
   return 0;
}
