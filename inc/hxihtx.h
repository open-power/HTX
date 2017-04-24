/* @(#)46  1.15.4.8  src/htx/usr/lpp/htx/inc/hxihtx.h, htx_libhtx, htxubuntu 6/1/15 07:43:26 */
/* Component = htx_libhtx_lx */

#ifndef __HTX_INCE_HXIHTX_H__
#define __HTX_INCE_HXIHTX_H__


/*
 *  hxihtx.h -- HTX Hardware Exerciser Include File
 *
 *    This include file defines data structures and
 *    keywords needed by Hardware Exerciser programs
 *    when they call the hxfupdate() function in
 *    libhtx.a.
 *
 */

#include <pthread.h>
#include <sys/types.h>

#define START     'S'
#define PSEUDOSTART     'P'
#define UPDATE    'U'
#define ERROR     'E'
#define RECONFIG  'R'
#define MESSAGE   'M'
#define FINISH    'F'
#define HE_STATUS 20

#define DEV_ID_MAX_LENGTH 40  /* defined in  hxiconv.h,hxihtx64.h, hxihtx.h  */

/******************************************************************
 * Malloc is not signal safe, this pre-processor directive would
 * change the malloc subroutine to signal safe HTX subroutine
 *
 *****************************************************************/
#ifndef __cplusplus 
#define malloc(size) htx_malloc(size)
#define free(ptr) htx_free(ptr)
#define localtime(p_time) htx_localtime(p_time)
#define ctime(p_time) htx_ctime(p_time)
#define localtime_r(timep,result) htx_localtime_r(timep,result)
#endif 

/* htx_data data structure "sev_code" enum definition...
 */
enum sev_code {
                HTX_SYS_HARD_ERROR = -10,
                HTX_SYS_SOFT_ERROR = (HTX_SYS_HARD_ERROR + 10),
                HTX_HE_HARD_ERROR = (HTX_SYS_SOFT_ERROR + 1),
                HTX_HE_MISCOMPARE = (HTX_SYS_SOFT_ERROR + 2),
                HTX_HE_SOFT_ERROR = (HTX_HE_HARD_ERROR + 3),
                HTX_SYS_INFO = (HTX_HE_SOFT_ERROR + 2),
                HTX_HE_INFO = (HTX_SYS_INFO + 1)
              };

/*
 * define the htx_data data structure
 */

#ifdef	__HTX_LINUX__
#define	MSG_TEXT_SIZE	(3912)
#else
#define MSG_TEXT_SIZE 4096        /* htx_data msg_text char array size    */
#endif

#define SYS_FORMAT_ROOM 240       /* room in msg_text for system info     */
#define MAX_TEXT_MSG (MSG_TEXT_SIZE - SYS_FORMAT_ROOM)  /* for HE use     */
#define ENV_VAR_SIZE 256 /* HTX_LOG_DIR env var size */

struct htx_msg_data{
	char			msg_text[MSG_TEXT_SIZE];
	char			sdev_id[DEV_ID_MAX_LENGTH];
	int				error_code;
	char			HE_name[32];
	enum sev_code	severity_code;
};

struct htx_data {
        char     sdev_id[DEV_ID_MAX_LENGTH];     /* Device id passed as first parameter  */
        char     run_type[4];     /* REG, EMC, or OTH                     */
        unsigned long long      bad_others;      /* Count of bad ioctl operations        */
        unsigned long long      bad_reads;       /* Count of bad read operations         */
        unsigned long long      bad_writes;      /* Count of bad write operations        */
        unsigned long long     bytes_read;      /* Total bytes read                     */
        unsigned long long     bytes_writ;      /* Total bytes written                  */
        unsigned long long      good_others;     /* Count of good ioctl operations       */
        unsigned long long      good_reads;      /* Count of good read operations        */
        unsigned long long      good_writes;     /* Count of good write operations       */
        unsigned long long    num_instructions; /* Number of instructions executed      */
        int      error_code;      /* Error Code (usually errno)           */
	    char loc_code[80];        /* To store the location code           */
        char serial_no[80];       /* Serial Number                        */
        char part_no[80];         /* Part Number                          */
        char dev_desc[80];        /* Device Description                   */
        char fru_no[80];          /* FRU Number                           */

        enum sev_code severity_code;
                                  /* HTX_HE_HARD_ERROR: for non-recoverable
                                   *                    HE errors.
                                   * HTX_HE_SOFT_ERROR: for recoverable HE
                                   *                    errors.
                                   * HTX_SYS_INFO: for HTX system info.
                                   * HTX_HE_INFO: for HE info.
                                   */

        char     HE_name[32];      /* Hardware Exerciser program name     */

        char     msg_text[MSG_TEXT_SIZE];
                                  /* character array for messages from the
                                   * Hardware Exerciser program.  The HE
                                   * program should use only MAX_TEXT_MSG
                                   * characters in its messages.
                                   * Additional space is reserved system
                                   * information.
                                   */
        int     msqid;                          /* HTX message queue id.    */
        int     sem_id;                         /* htx semaphore id.        */
        struct  htxshm_hdr      *p_shm_hdr;     /* pointer to shm header.   */
        struct  htxshm_HE       *p_shm_HE;      /* pointer to shm HE info.  */
        ushort  miscompare_count;
        ushort  test_id;
        int    mem_id;
        int hotplug_cpu;
        int hotplug_mem;
        int hotplug_io;
    	unsigned int pthread_id;
    	unsigned int TestCaseNo;
    	unsigned int allocseed;
    	unsigned int thirdseed;
	    unsigned int seedno;
	    unsigned int passno;
	    int      status;
	    unsigned int stanza;
	char htx_exer_log_dir[ENV_VAR_SIZE];
	char htx_log_dir[ENV_VAR_SIZE];
        int    rsvd3;
        int    rsvd4;
        int    rsvd5;

};

#include <htxlibdef.h>

#endif /* __HTX_INCE_HXIHTX_H__ */
