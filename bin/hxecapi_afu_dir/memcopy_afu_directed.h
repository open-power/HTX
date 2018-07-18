

#include <pthread.h>
#include <stdio.h>
#include <stdint.h>
#include <sys/types.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/errno.h>
#include <string.h>
#include <poll.h>
#include <hxihtx64.h>
#include <ctype.h>
#include <libgen.h>
#include <stdlib.h>
#include <malloc.h>

#ifdef DEBUGON 
	#define DEBUG 	1
#else 
	#define DEBUG  0 
#endif 

/* 
 * Memcopy AFU defines max copy size of 2KB
 */
#define MAX_COPY_SIZE					2048
#define MEMCOPY_COMMAND_COPY 			0x00 
#define MEMCOPY_COMMAND_INTERRUPT 		0x01
#define MEMCOPY_COMMAND_STOP 			0x02 
#define MEMCOPY_INTERRUPT_SOURCE 		0x1
#define MEMCOPY_COMMAND_INVALID(__cmd__) ((__cmd__ > 0x02) ? 1 : 0) 

#define CXL_EVENT_READ_FAIL 0xffff
#define CACHELINESIZE   128
/* Queue sizes other than 512kB don't seem to work */
#define QUEUE_SIZE  4095*CACHELINESIZE
#define ERR_IRQTIMEOUT  0x1
#define ERR_EVENTFAIL   0x2
#define ERR_MEMCMP  0x4

/* Default amount of time to wait (in seconds) for a test to complete */
#define TIMEOUT     120
#define KILL_TIMEOUT    5
/*
 * Memcopy AFU has 32 engines running in parallel 
 * So to match up, this exerciser creates 32 threads
 */ 
#define MAX_THREADS 32


#define MAX_RETRY     0x10

/*
 * Rule file path length
 */
#define PATH_SIZE 				MSG_TEXT_SIZE

/*
 * Rule ID String length
 */
#define MAX_STRING              MSG_TEXT_SIZE

/* 
 * Dumping Miscompare Information 
 */ 
#define DUMP_PATH 				"/tmp/"
#define MAX_MISCOMPARES 		8
#define MAX_MSG_DUMP 			0x10


/* 
 * Max stanza count 
*/
#define MAX_STANZA              MAX_THREADS

/********************************************************************************
 * Work Element Command Format
 *  
 * Valid Bit :  Set by software to indicate command is ready for processing.  
 *				Must be written last after rest of command has been written.  
 *				Cleared by software after command is complete.
 *
 * Wrap Bit  :	Wrap bit toggles each time through the FIFO.  
 *				This along with valid signifies if the command is valid.  
 *				This protects against the engine wrapping and working on old 
 *				commands before the software can clear the valid bit for commands 
 *				that are completed.
 * Command Bits : Command type to execute.
 *					0x00: Copy
 *					0x01: Interrupt
 *					0x02: Stop
 *					0x02-0x3F:  Invalid (Still to be defined)
 * Status Bits	:	Written by AFU upon completion of command.
 * 					Bit 8: Command is complete.
 *					Bit 9: Translation Fault ,Continue, restart mmio required
 *					Bit 10: Aerror
 *					Bit 11: Derror
 *					Bit 12: PSL Fault response (not expected)
 *					Bit 13: Failure (for interrupt command means invalid source number)
 *					Bit 14: Process terminated
 *					Bit 15: reserved
 * Length 		: For Copy Cmd: Number of bytes to copy.  Max of 2kB is all that 
 *					is supported in current version.  Length > 2kB is invalid and will 
 *					result in unknown behavior. Length has to be multiple of 64B 
 *					in current version.  Other lengths may be supported in future versions.
 *
 * Irq Src #    : For Interrupt Cmd: Indicates source number of interrupt to be generated. 
 *					Max src # is 2047.
 * Src EA		: Source address of data to be copied. Only valid for Copy command type.  
 *					Source EA must be aligned on half-line boundary. (bits 58:63 = 0).
 * Dest EA		:	Destination address for where to copy data to. 
 *					Only valid for Copy command type.  
 *					Destination EA must be aligned on a 64B boundary (bits 58:63 = 0).
 ***************************************************************************************/



struct memcpy_test_args {
    int processes;
    int loops;
    int buflen;
    int irq;
    int irq_count;
    int stop_flag;
    int timebase_flag;
};

/* 
 * Each threads data structure 
 */ 
struct thread_context { 

	/* 
	 * This thread ID returned from pthreat_create 
	 */ 
	pthread_t worker; 
	int32_t   worker_join_rc;

	uint32_t thread_id; 
    /*
     * /dev/memcopyXX file descriptor
     */
	int32_t fd ;
	
	uint64_t total_interrupts; 
	
	/* 
	 * Rules configurable test parameters 
	 * for this thread
	 */ 
	uint32_t 	num_oper; 
	uint32_t	command; 	
    char 		rule_id[MAX_STRING];
    uint32_t 	testcase:1;
    uint32_t 	bufsize;
    uint32_t 	compare:1;
    uint32_t 	num_threads ;
	uint32_t 	miscompare_count; 

	/* 
	 * Device name to open .. 
	 */ 
	char device[DEV_ID_MAX_LENGTH];

	/*
     * need a sync point b/w thread and SIGCAPI handler thread
     */
    pthread_mutex_t thread_lock;

	volatile uint32_t context_valid : 1; 

	struct cxl_afu_h *afu_h;
	int afu_fd;
	struct memcpy_test_args args;
};
	

/*
 * Each thread needs these arguments, clubbed together in same structure... 
 */ 
struct thno_htxd { 
	struct htx_data htx_d; 
	uint32_t thread_no;  
}; 

/*
 * Rule info strcuture used to store parsed rule file data
 */
struct rule_info {  
    char 		rule_id[MAX_STRING];  
    uint32_t 	testcase:1;  
    uint32_t 	bufsize;  
    uint32_t 	compare:1;  
    uint32_t 	num_threads;  
	uint32_t 	num_oper;
};
struct rule_info rule_data[MAX_STANZA];

/* 
 * Function declarations ... 
 */

int32_t compare_data(struct htx_data * htx_d, uint32_t cmd, uint32_t * miscompare_cnt, uint64_t * total_interrupts, char * src, char * dest, uint32_t buf_size);

void *tc_worker_thread(void * thread_context);

int get_rule_capi(struct htx_data * , char [], uint32_t *);

void print_rule_file_data(int);

int test_afu_memcpy(struct thread_context *, struct htx_data *);
