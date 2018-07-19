
#define _BSD_SOURCE 
#define __STDC_FORMAT_MACROS
#define _ISOC11_SOURCE
#define _GNU_SOURCE 

#include <sys/types.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <sys/select.h>
#include <sys/stat.h>
#include <inttypes.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <poll.h>
#include <endian.h>
#include <getopt.h>
#include <fcntl.h>
#include <libcxl.h>
#include "memcpy_afu.h"
#include "memcopy_afu_directed.h"


struct thread_context th_ctx[MAX_THREADS];
static uint32_t num_threads = 0; 
uint32_t exit_flag = 0; 
char afu_device[DEV_ID_MAX_LENGTH];

/* 
 * thread specific variables, applicable to worker threads 
 */
pthread_mutex_t create_thread_mutex;
pthread_attr_t worker_thread_attrs, excpt_thread_attrs;
pthread_cond_t create_thread_cond;
pthread_cond_t start_thread_cond;

int hxfcbuf_capi(struct htx_data * , uint32_t * , char * , char * , size_t );

/***********************************************************************
* SIGTERM signal handler                                               *
************************************************************************/

void SIGTERM_hdl (int sig, int code, struct sigcontext *scp)
{
    /* Dont use display function inside signal handler.
     * the mutex lock call will never return and process
     * will be stuck in the display function */
    exit_flag = 1;
}

int32_t
main(int32_t argc, char * argv[]) { 

	int32_t rc = 0, thread, i = 0 ;	
	struct thread_context * current_tctx = NULL; 
	char msg[MSG_TEXT_SIZE]; 
	char rule_filepath[PATH_SIZE];
	uint32_t stanza = 0, num_stanzas = 0;
	struct thno_htxd tctx_htxd; 
	struct sigaction sigvector;

	/* 
 	 * HTX Data Structure and rules file variable  
 	 */ 
	struct htx_data htx_d;

	sigemptyset(&(sigvector.sa_mask));
    sigvector.sa_flags = 0 ;
    sigvector.sa_handler = (void (*)(int)) SIGTERM_hdl;
    sigaction(SIGTERM, &sigvector, (struct sigaction *) NULL);

	
	/*
	 * Copy Command line argument passed in out internal data structure 
	 */ 
	 memset(&htx_d, 0x00, sizeof(struct htx_data)); 
	 if(argc < 4) { 
		printf("Usage /usr/lpp/htx/bin/hxecapi_afu_dir <dev_name> <REG/OTH> <rules_file> \n"); 
		return(0); 
	}

	if(argv[0]) strcpy(htx_d.HE_name, argv[0]);
    if(argv[1]) strcpy(htx_d.sdev_id, argv[1]);
    if(argv[2]) strcpy(htx_d.run_type,argv[2]);
	if(argv[3]) strcpy(rule_filepath, argv[3]);

	hxfupdate(START, &htx_d);

	strcpy(afu_device,basename(htx_d.sdev_id));
	
    sprintf(msg,"argc %d argv[0] %s argv[1] %s argv[2] %s argv[3] %s \n", argc, argv[0], argv[1], argv[2], argv[3]);
    hxfmsg(&htx_d, 0, HTX_HE_INFO, msg);

    pthread_attr_init(&worker_thread_attrs);
    pthread_attr_setdetachstate(&worker_thread_attrs, PTHREAD_CREATE_JOINABLE);
    pthread_attr_setscope(&worker_thread_attrs, PTHREAD_SCOPE_PROCESS);
    pthread_mutex_init(&create_thread_mutex, NULL);
    pthread_cond_init(&create_thread_cond, NULL);
    pthread_cond_init(&start_thread_cond, NULL);

    rc = get_rule_capi(&htx_d, rule_filepath, &num_stanzas);
	if(rc == -1) {
        sprintf(msg, "Rule file parsing failed. rc = %#x\n", rc);
        hxfmsg(&htx_d, rc, HTX_HE_HARD_ERROR, msg);
		return(rc);
    } 
	if(DEBUG) print_rule_file_data(num_stanzas);  

	/* 	
	 * LOOP infinitely if exer started with REG cmd line argument 
	 */
	do { 

		for(stanza = 0; stanza < num_stanzas; stanza ++) { 
	
			num_threads = rule_data[stanza].num_threads; 

			/* 
			 * Update Stanza count for SUP 
			 */ 
			htx_d.test_id = stanza + 1;
            hxfupdate(UPDATE, &htx_d);

			if(DEBUG) printf("stanza=%d \n", stanza); 
				
			/*
			 * Creating fresh threads .. 
			 * Grab mutex .. 
			 */ 
		    rc = pthread_mutex_lock(&create_thread_mutex);
    		if (rc) {
        		sprintf(msg, "pthread_mutex_lock(create_thread_mutex) failed with rc = %d, errno = %d \n", rc, errno);
        		hxfmsg(&htx_d, rc, HTX_HE_HARD_ERROR, msg);
        		return (rc);
    		}

			for(thread = 0; thread < num_threads; thread++) { 
	
				current_tctx = &th_ctx[thread]; 
				if(current_tctx->context_valid) { 
					sprintf(msg, "PID=%d: index(%d): Context still valid(%d) !! \n", getpid(), thread , current_tctx->context_valid);
					hxfmsg(&htx_d, current_tctx->context_valid, HTX_HE_HARD_ERROR, msg); 
					rc = -1; 
					break; 
				}
				/* 
				 * Populate this thread test case parameters from rules file 
				 */ 
				current_tctx->num_oper 			= 	rule_data[stanza].num_oper; 	
				current_tctx->thread_id 		= 	thread; 
				current_tctx->command 			= 	rule_data[stanza].testcase;
				current_tctx->compare			=	rule_data[stanza].compare;
				current_tctx->bufsize 			=	rule_data[stanza].bufsize;
				current_tctx->total_interrupts 	= 	0;
				memcpy(current_tctx->rule_id, rule_data[stanza].rule_id, MAX_STRING);
				/*memcpy(current_tctx->device, htx_d.sdev_id, DEV_ID_MAX_LENGTH); */
				sprintf(current_tctx->device, "/dev/cxl/%s",afu_device);

				/* 
				 * Populate arguments passed to thread via pthread_create() 
				 */ 
				memset(&tctx_htxd, 0x00, sizeof(struct thno_htxd));  
				tctx_htxd.thread_no = thread; 
				memcpy(&tctx_htxd.htx_d, &htx_d, sizeof(struct htx_data)); 
					
						

				/* 
				 * Create worker thread 
				 */
				if(DEBUG) printf("Creating thread - %d \n", thread); 
				rc = pthread_create(&current_tctx->worker, &worker_thread_attrs, tc_worker_thread, (void *)&tctx_htxd);
				if(rc ) {
					sprintf(msg, "failed to create worker thread with rc = %d, errnp = %d \n", rc, errno);
					hxfmsg(&htx_d, rc, HTX_HE_HARD_ERROR, msg); 
					break;
				}
				rc = pthread_cond_wait(&create_thread_cond, &create_thread_mutex);
				if(rc) {
					sprintf(msg, "[%s][%d] : pthread_cond_wait failed with rc = %d, errno = %d \n", __FUNCTION__, __LINE__, rc, errno);
					hxfmsg(&htx_d, rc, HTX_HE_HARD_ERROR, msg);	
					break;
				}
				if(rc) {
					break;
				}
			}
			/* 
			 * check if we are here due to error in creating threads 
			 */
			if(rc) {
				if(DEBUG) printf("No of thread actually created = %d \n", thread);
				for(i = 0; i < thread; i++) {
					sprintf(msg, " Cancelling thread tid = %d, id = %lu coz of error \n", i, th_ctx[i].worker);
					hxfmsg(&htx_d, 0, HTX_HE_SOFT_ERROR, msg); 
					pthread_cancel(th_ctx[i].worker);
				}
				return(rc);
			} else {
				/* 
				 * All the worker thread created successfully, start them 
				 */
				rc = pthread_cond_broadcast(&start_thread_cond);
				if(rc) {
					sprintf(msg, "pthread_cond_broadcast failed with rc = %d, errno = %d \n", rc, errno);
					hxfmsg(&htx_d, 0, HTX_HE_HARD_ERROR, msg);
					return(rc);
				}
				rc = pthread_mutex_unlock(&create_thread_mutex);
				if (rc) {
					sprintf(msg, "pthread_mutex_unlock failed with rc = %d, errno =  %d \n", rc, errno);
					hxfmsg(&htx_d, 0, HTX_HE_HARD_ERROR, msg);
					return(rc);
				}
				/* Worker thread started, wait for completion */
				for(i = 0; i < thread; i++) {
					rc = pthread_join(th_ctx[i].worker, (void *)&(th_ctx[i].worker_join_rc));
					if(rc) {
						break;
					}
				}
    		}
		} /* Off num_stanza loop */ 
		/* 
		 * Update cycle count for SUP 
		 */
		hxfupdate(FINISH, &htx_d);
        if(exit_flag)
            break;

	} while((rc = strcmp(htx_d.run_type, "REG") == 0) || (rc = strcmp(htx_d.run_type, "EMC") == 0));

   /*
	* Exiting exerciser, cleanup global attributes 
	*/ 	
	pthread_attr_destroy(&worker_thread_attrs);
    pthread_cond_destroy(&create_thread_cond);
    pthread_cond_destroy(&start_thread_cond);
    pthread_mutex_destroy(&create_thread_mutex);

	return(0); 
} 

int set_afu_master_psa_registers(int stopflag)
{
    struct cxl_afu_h *afu_master_h;
    struct memcpy_weq weq;
    __be64 reg_data;

    /* now that the AFU is started, lets set config options */
    afu_master_h = cxl_afu_open_dev("/dev/cxl/afu0.0m");
    if (afu_master_h == NULL) {
        perror("Unable to open AFU Master cxl device /dev/cxl/afu0.0m");
        return 1;
    }

    /* need a dummy work element queue to get things going */
    memcpy_init_weq(&weq, QUEUE_SIZE);

    if (cxl_afu_attach(afu_master_h,
               MEMCPY_WED(weq.queue, QUEUE_SIZE/CACHELINESIZE))) {
        perror("cxl_afu_attach (master)");
        return 1;
    }
    if (cxl_mmio_map(afu_master_h, CXL_MMIO_BIG_ENDIAN) == -1) {
        perror("Unable to map AFU Master problem state registers");
        return 1;
    }
    /* Set/Clear Bit 2 to stop AFU on Invalid Command */
    if (cxl_mmio_read64(afu_master_h, MEMCPY_AFU_PSA_REG_CTL, &reg_data) == -1) {/*[AFU Configuration Register (x0000)]*/
        perror("mmio read fail");
        return 1;
    }

    if (stopflag)
        reg_data = reg_data | 0x2000000000000000;
    else
        reg_data = reg_data & ~(0x2000000000000000);
    	if(DEBUG) printf("AFU PSA CTL REG: %#016lx", reg_data);
    if (cxl_mmio_write64(afu_master_h, MEMCPY_AFU_PSA_REG_CTL, reg_data) == -1) {
        perror("mmio write fail");
        return 1;
    }
    cxl_afu_free(afu_master_h);

    return 0;
}

void *
tc_worker_thread(void * args) {

    int rc = 0;
	char msg[MSG_TEXT_SIZE]; 
	struct thread_context * tctx; 
	struct htx_data	htx_d; 


	struct thno_htxd * tctx_htxd = (struct thno_htxd *)args; 
	memcpy(&htx_d, &tctx_htxd->htx_d, sizeof(struct htx_data)); 
	tctx = &th_ctx[tctx_htxd->thread_no];

    rc = pthread_mutex_lock(&create_thread_mutex);
    if (rc) {
        sprintf(msg, "%s : pthread_mutex_lock failed with rc = %d, errno = %d \n", __FUNCTION__, rc, errno);
		hxfmsg(&htx_d, rc, HTX_HE_HARD_ERROR, msg);
        pthread_exit(&rc);
    }

    /* 
	 * notify main thread to proceed creating other worker threads 
	 */
    rc = pthread_cond_broadcast(&create_thread_cond);
    if (rc) {
        sprintf(msg, "%s :  pthread_cond_broadcast failed with rc = %d, errno = %d \n", __FUNCTION__, rc, errno);
		hxfmsg(&htx_d, rc, HTX_HE_HARD_ERROR, msg);
        pthread_exit(&rc);
    }
    /* 
	 * wait for start notification from main thread 
	 */
    rc = pthread_cond_wait(&start_thread_cond, &create_thread_mutex);
    if (rc) {
        sprintf(msg, " %s : pthread_cond_wait failed with rc = %d, errno = %d \n", __FUNCTION__, rc , errno);
		hxfmsg(&htx_d, rc, HTX_HE_HARD_ERROR, msg);
        pthread_exit(&rc);
    }

    rc = pthread_mutex_unlock(&create_thread_mutex);
    if (rc) {
        sprintf(msg, "%s : pthread_mutex_unlock failed with rc = %d, errno = %d \n", __FUNCTION__, rc , errno);
		hxfmsg(&htx_d, rc, HTX_HE_HARD_ERROR, msg);
        pthread_exit(&rc);
    }

	set_afu_master_psa_registers(0);

	/*
	 * Memcopy test case  
	 */
	rc = test_afu_memcpy(tctx, &htx_d);
    if(rc) {
       if(DEBUG) printf("thread with id = %d returned with rc = %d \n", tctx->thread_id, rc);
       pthread_exit(&rc);
    }else {
		printf("Successful thread number=%d \n",tctx->thread_id);
	}
	pthread_exit(NULL);
}


int test_afu_memcpy(struct thread_context * tctx, struct htx_data * htx_d)
{
    __u64 afu_tb, wed, status, process_handle_memcpy, psr_stat;
	char msg[MSG_TEXT_SIZE];
    int process_handle_ioctl;
    pid_t pid;
    int i, j, ret = 0, t;
	int l = 0;
    struct memcpy_weq weq;
    struct memcpy_work_element memcpy_we, irq_we, *queued_we;
    struct cxl_event event;
    struct timeval timeout;
    struct timeval start, end;
    fd_set set;
	long cnt_int=0;
	char *src;
	char *dst;
	size_t bufsize;
	int oper = 0;
	int rc = 0;
	int locked = 0;
	int afu_error = 0;
	int32_t afu_restart = 0;
	int retry;

	tctx->miscompare_count = 0;

	bufsize	= tctx->bufsize; 
	
	/* Allocate memory areas for afu to copy to/from */
    src = (char *) aligned_alloc(CACHELINESIZE/2, bufsize);
	if(src == NULL) {
		sprintf(msg, "[th:%d]:[oper:%d]:[aligned_alloc for src failed]\n", tctx->thread_id, oper);
		hxfmsg(htx_d, 1, HTX_HE_HARD_ERROR, msg);
        return 1;
	}
	
    dst = (char *) aligned_alloc(CACHELINESIZE/2, (size_t)bufsize);
	if(dst == NULL) {
        sprintf(msg, "[th:%d]:[oper:%d]:[aligned_alloc for dst failed]\n", tctx->thread_id, oper);
		hxfmsg(htx_d, 1, HTX_HE_HARD_ERROR, msg);
        return 1;
    }
   
	pid = getpid();

	/*Open AFU device*/
    tctx->afu_h = cxl_afu_open_dev(tctx->device);
    if (tctx->afu_h == NULL) {
		sprintf(msg, "[th:%d]:[oper:%d]:[Unable to open cxl device %s. errno =%#x Exiting..]\n", tctx->thread_id, oper, tctx->device, errno);
		hxfmsg(htx_d, 1, HTX_HE_HARD_ERROR, msg);
        return 1;
    }
	
	/*Interrupt Command*/
    if(tctx->command == MEMCOPY_COMMAND_INTERRUPT) {
		tctx->args.irq        = 1;
		cxl_get_irqs_min(tctx->afu_h,&cnt_int);
		tctx->args.irq_count  = cnt_int; 
	}else {
		tctx->args.irq              = 0;
		tctx->args.irq_count        = -1;
	}

	tctx->args.stop_flag		= 0;
	tctx->args.timebase_flag	= 0;

	/* Get AFU file descriptor*/
    tctx->afu_fd = cxl_afu_fd(tctx->afu_h);

	/*Initialize WEQ*/
    memcpy_init_weq(&weq, QUEUE_SIZE);

    /* Point the work element descriptor (wed) at the weq */
    wed = MEMCPY_WED(weq.queue, QUEUE_SIZE/CACHELINESIZE);
	if(DEBUG) printf("WED = 0x%"PRIx64" for PID = %d", wed, pid);
    sprintf(msg, "[th:%d]:[oper:%d]:[WED = 0x%"PRIx64" for PID = %d]\n", tctx->thread_id, oper, wed, pid);
	hxfmsg(htx_d, 0, HTX_HE_INFO, msg);

    /* Setup a work element in the queue */
	if(tctx->command == MEMCOPY_COMMAND_COPY) {
    	memcpy_we.cmd = MEMCPY_WE_CMD(0, MEMCPY_WE_CMD_COPY);/*[MEMCPY_WE_CMD(valid bit,command)].COPY CMD-0*/
	    memcpy_we.status = 0;
    	memcpy_we.length = htobe16((uint16_t)bufsize);
	    memcpy_we.src = htobe64((uintptr_t)src);
    	memcpy_we.dst = htobe64((uintptr_t)dst);
	}

    /* Setup the interrupt work element */
	if(tctx->command == MEMCOPY_COMMAND_INTERRUPT) {
	    irq_we.cmd = MEMCPY_WE_CMD(1, MEMCPY_WE_CMD_IRQ);/*INTERRUPT-129*/
    	irq_we.status = 0;
	    irq_we.length = htobe16((uint16_t)tctx->args.irq); /*IRQ Source number*/
		irq_we.src = 0;
	    irq_we.dst = 0;
	}

    /* Start the AFU */
    if (tctx->args.irq_count == -1)
        ret = cxl_afu_attach(tctx->afu_h, wed);
    else
        ret = cxl_afu_attach_full(tctx->afu_h, wed, tctx->args.irq_count, 0);
    if (ret) {
        sprintf(msg, "[th:%d]:[oper:%d]:[cxl_attach failed. errno =%#x Exiting..]\n", tctx->thread_id, oper, errno);
		hxfmsg(htx_d, 1, HTX_HE_HARD_ERROR, msg);
        return 1;
    }

	/*Gets the process element associated with an open AFU handle*/
    process_handle_ioctl = cxl_afu_get_process_element(tctx->afu_h);
    if (process_handle_ioctl < 0) {
        sprintf(msg, "[th:%d]:[oper:%d]:[process_handle_ioctl failed. errno =%#x Exiting..]\n", tctx->thread_id, oper, errno);
		hxfmsg(htx_d, 1, HTX_HE_HARD_ERROR, msg);
        return 1;
    }

    if (cxl_mmio_map(tctx->afu_h, CXL_MMIO_BIG_ENDIAN) == -1) {
        sprintf(msg, "[th:%d]:[oper:%d]:[Unable to map problem state registers. errno =%#x Exiting..]\n", tctx->thread_id, oper, errno);
		hxfmsg(htx_d, 1, HTX_HE_HARD_ERROR, msg);
        return 1;
    }

	/*Read of this register will return the process handle of AFU.[Process Handle Register (x0008)]*/
    if (cxl_mmio_read64(tctx->afu_h, MEMCPY_PS_REG_PH, &process_handle_memcpy) == -1) {
        sprintf(msg, "[th:%d]:[oper:%d]:[Unable to read mmaped space. errno =%#x Exiting..]\n", tctx->thread_id, oper, errno);
		hxfmsg(htx_d, 1, HTX_HE_HARD_ERROR, msg);
        return 1;
    }
    process_handle_memcpy = process_handle_memcpy >> 48;

    if ((process_handle_memcpy == 0xdead) ||
        (process_handle_memcpy == 0xffff))  {
        sprintf(msg, "[th:%d]:[oper:%d]:[Bad process handle. errno =%#x Exiting..]\n", tctx->thread_id, oper, errno);
		hxfmsg(htx_d, 1, HTX_HE_HARD_ERROR, msg);
        return 1;
    }
    assert(process_handle_memcpy == process_handle_ioctl);

    /* Initialise source buffer with value */
    for (i = 0; i < bufsize; i++) {
		if(i==0)
			*(src + i) = (char)tctx->thread_id;
		else
	        *(src + i) = 0xAA;
	}

    FD_ZERO(&set);/*Clears set*/
    FD_SET(tctx->afu_fd, &set);/*Adds a given file descriptor from a set*/

    if (tctx->args.timebase_flag) {
        /* Request an update of the AFU TB.[Timebase Register (x0040)]. */
        if (cxl_mmio_write64(tctx->afu_h, MEMCPY_PS_REG_TB, 0x1234ULL) == -1){
            sprintf(msg, "[th:%d]:[oper:%d]:[MMIO write to AFU TB register failed]\n", tctx->thread_id, oper);
			hxfmsg(htx_d, 0, HTX_HE_INFO, msg);
		}

        /* Read the AFU TB, retry until non zero */
        i = 0;
        do {
            if (i++ > 10000000) {
                sprintf(msg, "[th:%d]:[oper:%d]:[Timeout waiting for AFU TB update]\n", tctx->thread_id, oper);
				hxfmsg(htx_d, -1, HTX_HE_HARD_ERROR, msg);
                return -1;
			}
			/*[Timebase Register (x0040)]. */
            if (cxl_mmio_read64(tctx->afu_h, MEMCPY_PS_REG_TB,
                        &afu_tb) == -1){
                sprintf(msg, "[th:%d]:[oper:%d]:[MMIO read from AFU TB register failed]\n", tctx->thread_id, oper);
				hxfmsg(htx_d, 0, HTX_HE_INFO, msg);	
			}
        } while (!afu_tb);

        t = (afu_tb * 1000000) / 512000000;
        sprintf(msg, "[th:%d]:[oper:%d]:[AFU time (usec) = %d]\n", tctx->thread_id, oper, t);
		hxfmsg(htx_d, 0, HTX_HE_INFO, msg);
    }

    gettimeofday(&start, NULL);

    if (tctx->args.timebase_flag) {
        t = start.tv_sec*1000000 + start.tv_usec;
        sprintf(msg, "[th:%d]:[oper:%d]:[core time (usec) = %d]\n", tctx->thread_id, oper, t);
        hxfmsg(htx_d, 0, HTX_HE_INFO, msg);
    }

    for (oper = 0; oper < tctx->num_oper; oper++) {
		if(exit_flag) {
            break;
        }

        ret = 0;
		if(tctx->command == MEMCOPY_COMMAND_COPY) {
	        queued_we = memcpy_add_we(&weq, memcpy_we);
		}
        if (tctx->args.irq)
            queued_we = memcpy_add_we(&weq, irq_we);

        queued_we->cmd |= MEMCPY_WE_CMD_VALID; /*Set valid command to start Memcopy operation*/

        /* If stop flag turned on, need to restart this CTX in the MCP AFU */
		/*[Process Control Register (Application Access) (x0018)]*/
        if (tctx->args.stop_flag)
            if (cxl_mmio_write64(tctx->afu_h, MEMCPY_PS_REG_PCTRL,
                         0x8000000000000000ULL) == -1)
                return 1;

        if (tctx->args.irq) {
            /* Set timeout to 1 second */
            timeout.tv_sec = 1;
            timeout.tv_usec = 0;
            if (select(tctx->afu_fd+1, &set, NULL, NULL, &timeout) <= 0) {
				if(cxl_mmio_read64(tctx->afu_h, MEMCPY_PS_REG_STATUS, &psr_stat)== -1){
            		sprintf(msg,"[th:%d]:[oper:%d]:[mmio read failure]\n", tctx->thread_id, oper);
					hxfmsg(htx_d, 0, HTX_HE_INFO, msg);
		        }
                sprintf(msg,"[th:%d]:[oper:%d]:[Timeout waiting for interrupt! pe: %i.PSR Register Contents=%016llX]\n", tctx->thread_id, oper, process_handle_ioctl, (unsigned long long)psr_stat);
				hxfmsg(htx_d, -1, HTX_HE_HARD_ERROR, msg);
                return -1;
            } else {
                if (cxl_read_expected_event(tctx->afu_h, &event, CXL_EVENT_AFU_INTERRUPT, tctx->args.irq)) {
                    sprintf(msg, "[th:%d]:[oper:%d]:[Failed reading expected Interrupt event]\n", tctx->thread_id, oper);
					hxfmsg(htx_d, -1, HTX_HE_HARD_ERROR, msg);
	                return -1;
                }
            }
            do {
                /* Make sure AFU is waiting on restart */
                if(DEBUG) printf("[th:%d]:[oper:%d]:[Waiting on AFU to wait for restart]\n", tctx->thread_id, oper); 
                cxl_mmio_read64(tctx->afu_h, MEMCPY_PS_REG_STATUS,
                        &status); /*[Process Status Register  (x0010)]*/
			} while (!(status & 0x0800000000000000));
            /* do restart */

			if(DEBUG) printf("[th:%d]:[oper:%d]:[Restarting AFU due to Interrupt]\n", tctx->thread_id, oper);
			/*[Process Status Register  (x0010)]*/
            cxl_mmio_read64(tctx->afu_h, MEMCPY_PS_REG_STATUS,
                        &status);
			/*[Process Control Register (Application Access) (x0018)].
			  Issues a restart by setting bit 0.*/
         	cxl_mmio_write64(tctx->afu_h, MEMCPY_PS_REG_PCTRL,
                     0x8000000000000000);

        }/*Interrupt Case end*/

        for (j = 100000000; j > 0 && !(queued_we->status & MEMCPY_WE_STAT_COMPLETE); j--) {
			if(exit_flag)
	            break;
			
			if(queued_we->status & MEMCPY_WE_STAT_TRANS_FAULT){
				sprintf(msg, "[th:%d]:[oper:%d]:[AFU Translation fault]\n", tctx->thread_id, oper);
				hxfmsg(htx_d, 0, HTX_HE_INFO, msg);
				afu_error = 1;
				afu_restart++;
				break;
			}else if(queued_we->status & MEMCPY_WE_STAT_AERROR){
                sprintf(msg, "[th:%d]:[oper:%d]:[AFU Error]\n", tctx->thread_id, oper);
				hxfmsg(htx_d, 0, HTX_HE_INFO, msg);
				afu_error = 1;
				break;
            }else if(queued_we->status & MEMCPY_WE_STAT_DERROR){
				sprintf(msg, "[th:%d]:[oper:%d]:[AFU Invalid memory reference]\n", tctx->thread_id, oper);
				hxfmsg(htx_d, 0, HTX_HE_INFO, msg);
				afu_error = 1;
				break;
			}
		}
		if (j == 0){
            sprintf(msg, "[th:%d]:[oper:%d]:[Timeout polling for completion.Retrying..]\n", tctx->thread_id, oper);
			hxfmsg(htx_d, 0, HTX_HE_INFO, msg);
			retry = MAX_RETRY;
			do {
				if(exit_flag)
	                break;

				if(!(queued_we->status & MEMCPY_WE_STAT_COMPLETE)){
					if(queued_we->status & MEMCPY_WE_STAT_TRANS_FAULT){
            	    	sprintf(msg, "[th:%d]:[oper:%d]:[AFU Translation fault]\n", tctx->thread_id, oper);
        	    	    hxfmsg(htx_d, 0, HTX_HE_INFO, msg);
    	    	        afu_error = 1;
	    	            afu_restart++;
						break;
	            	}else if(queued_we->status & MEMCPY_WE_STAT_AERROR){
            	    	sprintf(msg, "[th:%d]:[oper:%d]:[AFU Error]\n", tctx->thread_id, oper);
        	    	    hxfmsg(htx_d, 0, HTX_HE_INFO, msg);
    	    	        afu_error = 1;
						break;
	    	        }else if(queued_we->status & MEMCPY_WE_STAT_DERROR){
	                	sprintf(msg, "[th:%d]:[oper:%d]:[AFU Invalid memory reference]\n", tctx->thread_id, oper);
                		hxfmsg(htx_d, 0, HTX_HE_INFO, msg);
            		    afu_error = 1;
						break;
            		}else {
						sleep(MAX_RETRY - retry + 1);
	                    retry --;
					}
				}else {
					break;
				}
			}while(retry);
			
			if(retry == 0) {
				/*
				cxl_mmio_read64(tctx->afu_h, MEMCPY_PS_REG_STATUS, &psr_stat);*//*[Process Status Register  (x0010)]
				*/
                sprintf(msg, "[th:%d]:[oper:%d]:[%s:%d][AFU seems to be HUNG for %d secs]\n",
                                    tctx->thread_id, oper, __FUNCTION__, __LINE__, (MAX_RETRY *(MAX_RETRY - 1)));
                hxfmsg(htx_d, rc, HTX_HE_SOFT_ERROR, msg);
                goto cleanup_AFU;
            }
		}

		if(afu_restart){
			sprintf(msg, "[th:%d]:[oper:%d]:[Restarting AFU...]\n", tctx->thread_id, oper);
			hxfmsg(htx_d, 0, HTX_HE_INFO, msg);
			cxl_mmio_read64(tctx->afu_h, MEMCPY_PS_REG_STATUS,
                        &psr_stat);/*[Process Status Register  (x0010)]*/

			/*Do restart[Process Control Register (Application Access) (x0018)].
			  Issues a restart by setting bit 0.*/
			cxl_mmio_write64(tctx->afu_h, MEMCPY_PS_REG_PCTRL,
                     0x8000000000000000);
			afu_restart=0;
		}

		/*
		*(src + 1) = 0xAB;	
		*/

		if(!afu_error) {
			if(tctx->compare) {
				if(tctx->command == MEMCOPY_COMMAND_COPY) {
    	    		rc = compare_data(htx_d, memcpy_we.cmd, &tctx->miscompare_count, &tctx->total_interrupts, src, dst, bufsize);
					if(rc) {
       	        		sprintf(msg, "[th:%d]:[oper:%d]:[memcopy compare data mismatch]\n",tctx->thread_id, oper);
						hxfmsg(htx_d, 0, HTX_HE_INFO, msg);
        	            htx_d->bad_writes += 1;
    	            } else {
						if((oper % 50) == 0) {
                       		htx_d->good_writes += 1;
                    	   	htx_d->bytes_writ += bufsize;
						}
            	    }
        	        hxfupdate(UPDATE, htx_d);
				} else {
					rc = compare_data(htx_d, 1,  &tctx->miscompare_count, &tctx->total_interrupts, src, dst, irq_we.length);
					if(rc){
        		        sprintf(msg, "[th:%d]:[oper:%d]:[compare data mismatch]\n",tctx->thread_id, oper);
						hxfmsg(htx_d, 0, HTX_HE_INFO, msg);
    	        	    return(rc);
		            }else{
						if(DEBUG) printf("[th:%d]:[oper:%d]:[Total interrupts received=%ld]\n",tctx->thread_id, oper, tctx->total_interrupts);
					}
				}
			}   
		}

		afu_error = 0;

		if(tctx->command == MEMCOPY_COMMAND_COPY) {
			i=0;
			for (i = 0; i < bufsize; i++){
				if(i==0)
					*(dst + i) = (char)tctx->thread_id;
				else
        			*(dst + i) = 0xBB;
			}	
		}
		
		/*printf(msg, "[th:%d]:[oper:%d]:[src:%#llx]:[dst:%#llx]:[bufsize:%#x]\n", tctx->thread_id, oper, src, dst, bufsize);*/

    }/*End Of Num Oper Loop*/

    gettimeofday(&end, NULL);
    t = (end.tv_sec - start.tv_sec)*1000000 + end.tv_usec - start.tv_usec;
    sprintf(msg, "[th:%d]:[oper:%d]:[%d loops in %d uS (%0.2f uS per loop)]\n", tctx->thread_id, oper, tctx->num_oper, t, ((float) t)/tctx->num_oper);
	hxfmsg(htx_d, 0, HTX_HE_INFO, msg);

cleanup_AFU :

	/* Unmap AFU*/
	/*    
	ret = cxl_mmio_unmap (tctx->afu_h);

    if(ret < 0 ) {
	    sprintf(msg, "cxl_mmio_unmap for device failed errno = %d %s. \n",errno,strerror(errno));
        hxfmsg(htx_d, -1, HTX_HE_HARD_ERROR, msg);
        return -1;
    }
	*/

	cxl_afu_free(tctx->afu_h);

	free(src);
	if(src != NULL){
		src = NULL;
	}
	free(dst);
	if(dst != NULL){
		dst = NULL;
	}
    
	return ret;

}

int32_t
compare_data(struct htx_data * htx_d, uint32_t cmd, uint32_t * miscompare_count, uint64_t * total_interrupts, char * src, char * dest, uint32_t buf_size) { 

	if(cmd == MEMCOPY_COMMAND_INTERRUPT) { 
		*total_interrupts = *total_interrupts + 1 ; 
		return(0); 
	} 

	int32_t rc = 0; 
	rc = hxfcbuf_capi(htx_d, miscompare_count, src, dest, buf_size); 
	
	return(rc);
}
 

int hxfcbuf_capi(struct htx_data * htx_d, uint32_t * cnt, char * wbuf, char * rbuf, __u64 len) {
    char path[MAX_STRING];
    char s[3];
    char work_str[MAX_STRING], misc_data[MAX_STRING];
    char * msg_ptr = NULL;
    uint32_t i = 0, j = 0, mis_flag = FALSE;
    int32_t rc = 0;

    if(memcmp(wbuf, rbuf, len)) {
        while ((mis_flag == FALSE) && (i < len)) {
            if (wbuf[i] != rbuf[i]) {
                mis_flag = TRUE;
            } else {
                i++;
            }
        }
    }
    if (mis_flag == TRUE) {         /* problem with the compare */
        rc = -1;
        msg_ptr = misc_data;        /* show bad compare */
		sprintf(msg_ptr, "Found miscompare at %d position.\n Miscompare Buffer address [rbuf:%lu] [wbuf:%lu]\n",i, (unsigned long)&rbuf[i], (unsigned long)&wbuf[i]);
		hxfmsg(htx_d, 0, HTX_HE_INFO, msg_ptr);
        sprintf(msg_ptr, "Miscompare at displacement = %d(%#x). Transfer Size=%lu(%#lx) \n",i, i, len, len);
        strcat(msg_ptr, "Expected Data = ");

        memset(work_str, '\0', MAX_STRING);
        for (j = i; ((j - i) < MAX_MSG_DUMP) && (j < len); j++) {
        #ifdef  __HTX_LINUX__
            sprintf(s, "%x", wbuf[j]);
        #else
            sprintf(s, "%0.2x", wbuf[j]);
        #endif
            strcat(work_str, s);
        }
        sprintf(msg_ptr + strlen(msg_ptr), "%s\n", work_str);
        strcat(msg_ptr, "Actual Data   = ");

        memset(work_str, '\0', MAX_STRING);
        for (j = i; ((j - i) < MAX_MSG_DUMP) && (j < len); j++) {
        #ifdef  __HTX_LINUX__
        sprintf(s, "%x", rbuf[j]);
        #else
            sprintf(s, "%0.2x", rbuf[j]);
        #endif
             strcat(work_str, s);
        }
        sprintf(msg_ptr + strlen(msg_ptr), "%s\n", work_str);

        if (*cnt < MAX_MISCOMPARES) {

       	  /*
           * Copy write and read buffers to dump file.
           */

           (*cnt)++;

          /*
           * Expected buffer path
           */
            strcpy(path, DUMP_PATH);
            strcat(path, "htx_dev_cxl_");
            strcat(path, afu_device);
            strcat(path, ".expected");
            sprintf(work_str, "_%lu_%d", pthread_self(), *cnt);
            strcat(path, work_str);
            hxfsbuf(wbuf, len, path, htx_d);
            sprintf(msg_ptr + strlen(msg_ptr), "The miscompare buffer dump files are %s,  ", path);
            /*
             * Actual buffer path
             */

            strcpy(path, DUMP_PATH);
            strcat(path, "htx_dev_cxl_");
            strcat(path, afu_device);
            strcat(path, ".actual");
            sprintf(work_str, "_%lu_%d", pthread_self(), *cnt);
            strcat(path, work_str);
            hxfsbuf(rbuf, len, path, htx_d);
            sprintf(msg_ptr + strlen(msg_ptr), " and %s\n", path);
        } else {
            sprintf(work_str, "The maximum number of saved miscompare \
                                    buffers (%d) have already\nbeen saved.  The read and write buffers for this \
                                    miscompare will\nnot be saved to disk.\n", MAX_MISCOMPARES); 
            strcat(msg_ptr, work_str);
        }
        hxfmsg(htx_d, i, HTX_HE_MISCOMPARE, msg_ptr);
    } /* endif */
    return(rc);

} /* hxfcbuf() */
