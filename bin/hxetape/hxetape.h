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

/******************************************************************************
 * COMPONENT_NAME: exer_tape
 *
 * MODULE NAME: hxetape.h
 *
 *
 * DESCRIPTION =    Common header file for the HTX tape exerciser.
 *
 * NOTE: Each file in the source code list for this exerciser has an
 *       "internal flags" number range associated with it.  This is a
 *       three digit hex number that gets printed in the "error" field
 *       from the HTX message processor.    The number is in hex because
 *       that is the format HTX uses, and it is helpful to avoid the
 *       necessity of converting bases when debugging with this feature.
 *       All functions within a source file use unique error numbers
 *       within the range to aid in identifying exactly where a message
 *       originated from.  The range is identified at the start of every
 *       source file.  Currently, the internal flag ranges and their
 *       associated source files are:
 *
 *      0x100 - 0x1ff:  hxetape.c       main tape exerciser program
 *      0x200 - 0x2ff:  get_rule.c      rules file interpreter routine
 *      0x300 - 0x3ff:  proc_rule.c     rule processor routine
 *      0x400 - 0x4ff:  buf_oper.c      read/write buffer routines
 *      0x500 - 0x5ff:  io_oper.c       input/output operations routines
 *      0x600 - 0x6ff:  prt_msg.c       message formatter/transmit routines
 *****************************************************************************/

#ifndef _H_HXETAPE_
#define _H_HXETAPE_

#ifndef __HTX_LINUX__
#include <sys/types.h>
#ifndef __64BIT__
#include "hxihtx.h"
#else
#include "hxihtx64.h"
#endif
#else
#include <hxihtx.h>
#include <sys/signal.h>
#endif

#define BLKSIZ (512)         /* read/write data block size in bytes      */
#define BUF_SIZE  1024 * 1024 /* read/write buffer size                  */
#define HARD 1               /* Messages with sev code other than 7 are  */
#define SOFT 4               /* reproduced in the htxerrs file AND WILL  */
#define SYSERR 0             /* HANG the exerciser within HTX if the     */
#define INFO 7               /* "stop-on-error" mode is active!!!        */

#ifndef __HTX_LINUX__
#pragma mc_func trap { "7c810808" }
#pragma reg_killed_by 	trap
#else
/*
 * for now just blank it
 * later on when do_trap_htx is ready
 * put the defination here
 */
#define	trap(_F_, _ARGS_...)
#define	reg_killed_by	trap

#define STIOCTOP MTIOCTOP
#define STREW MTREW
#define STERASE MTERASE
#define STWEOF MTWEOF
#define STFSF MTFSF
#define STRSF MTBSF
#define STFSR MTFSR
#define STRSR MTBSR
#define STOFFL MTOFFL
#define stop mtop
#define st_op mt_op
#define st_count mt_count

struct on_exit_st{
	unsigned int	blk_size;
	char			dev_id[64];
};

struct scsi_mapping{
	struct scsi_mapping *next;
	char sg[32];
	char st[32];
};

#endif

struct ruleinfo {
  char  rule_id[9];      /* Rule Id                                      */
  char  pattern_id[9];   /* pattern library id                           */
  int   num_oper;        /* number of operations to be performed         */
  char  oper[7];         /* R,W,RC,etc.....                              */
  int   num_blks;        /* length of data to read/write in blocks       */
  int   fildes;          /* file descriptor for device to be exercised   */
  unsigned int dlen;     /* length of data to read/write in bytes        */
  long  tot_blks;        /* Total number of blocks on tape               */
  int   u_sleep;         /* Number of Seconds to Sleep on Unload         */
  int   source_id;       /* Source Address                               */
  int   dest_id;         /* Destination Address                          */
  int   source_id1;      /* Second Source Address                        */
  int   dest_id1;        /* Second Destination Address                   */
  char  chs_file[32];    /* Special File Name                            */
  char  cmd_list[250];   /* array to hold command from rules file        */
  unsigned short seed[3];
  char VBS_seed_type;
} ;

struct blk_num_typ {
  int   in_rule;         /* relative block number within current rule op */
  int   in_file;         /* current block number position on tape        */
} ;

struct tapeinfo{
     short   st_op;
     daddr_t st_count;
} ;

/*
 * Function declaration for buf_oper.c
 */
void bldbuf(unsigned short *wbuf, unsigned int dlen, char *pattern_id,
				struct blk_num_typ *pblk_num);
int cmpbuf(struct htx_data *phtx_info, struct ruleinfo *prule_info,
				int loop,struct blk_num_typ *pblk_num, char wbuf[], char rbuf[]);

void init_seed(unsigned short seed[]);
/*
 * Function declaration for get_rule.c
 */
int get_rule(struct htx_data *phtx_info, struct ruleinfo *prule_info, char *rules_name);
void set_defaults(struct ruleinfo *prule_info);
int get_line(char s[], int lim);

/*
 * Function declaration for hxetape.c
 */
void finish_msg(struct htx_data *phtx_info, char *msg_text);
void start_msg(struct htx_data *phtx_info, struct ruleinfo *prule_info, char *msg_text);
void finish_stanza(struct htx_data *phtx_info, struct ruleinfo *prule_info, double total,
				char *msg_text);

int e_notation(double d, char *s);
void create_scsi_mapping(struct htx_data *);
/*
 * Function declaration for io_oper.c
 */
int write_tape(struct htx_data *phtx_info, struct ruleinfo *prule_info, int loop,
				struct blk_num_typ *pblk_num, char *wbuf);

int read_tape(struct htx_data *phtx_info, struct ruleinfo *prule_info,
				int loop, struct blk_num_typ *pblk_num, char *rbuf);


int rewind_tape(struct htx_data *phtx_info, struct ruleinfo *prule_info,
				int loop, struct blk_num_typ *pblk_num);

int weof_tape(struct htx_data *phtx_info, struct ruleinfo *prule_info, int loop,
				struct blk_num_typ *pblk_num);

int search_file(struct htx_data *phtx_info, struct ruleinfo *prule_info, int loop,
	            struct blk_num_typ *pblk_num);

int search_rec(struct htx_data *phtx_info, struct ruleinfo *prule_info, int loop,
				struct blk_num_typ *pblk_num);

int diag_tape(struct htx_data *phtx_info, struct ruleinfo *prule_info, int loop,
		        struct blk_num_typ *pblk_num);

int erase_tape(struct htx_data *phtx_info, struct ruleinfo *prule_info, int loop,
        		struct blk_num_typ *pblk_num);

int do_sleep(struct htx_data *phtx_info, struct ruleinfo *prule_info, int loop,
				struct blk_num_typ *pblk_num);

int close_open(struct htx_data *phtx_info, struct ruleinfo *prule_info, int loop,
				struct blk_num_typ *pblk_num);

int tclose(struct htx_data *phtx_info, struct ruleinfo *prule_info, int loop,
				struct blk_num_typ *pblk_num);

int topen(struct htx_data *phtx_info, struct ruleinfo *prule_info, int loop,
				struct blk_num_typ *pblk_num);

int write_eot(struct htx_data *phtx_info, struct ruleinfo *prule_info, int loop,
				struct blk_num_typ *pblk_num, char *wbuf);

int read_eot(struct htx_data *phtx_info, struct ruleinfo *prule_info, int loop,
				struct blk_num_typ *pblk_num, char *wbuf, char *rbuf);

int read_teot(struct htx_data *phtx_info, struct ruleinfo *prule_info, int loop,
				struct blk_num_typ *pblk_num, char *wbuf, char *rbuf);

int prt_req_sense(struct htx_data *phtx_info, struct ruleinfo * prule_info,
				int loop, struct blk_num_typ *pblk_num);

int get_dd_blksize(char *sdev_id, char *dev_type, struct htx_data *phtx_info,
				struct ruleinfo *prule_info, struct blk_num_typ *pblk_num);

int set_dbug(struct htx_data *phtx_info, struct ruleinfo *prule_info,
				struct blk_num_typ *pblk_num);

int set_bs(struct htx_data *pHTX, struct ruleinfo *pRule, int size, int marker);

int VBS_Write(struct htx_data *pHTX, struct ruleinfo *pRule,
				struct blk_num_typ *pblk_num);

int VBS_Read(struct htx_data *pHTX, struct ruleinfo *pRule,
				struct blk_num_typ *pblk_num);

int VBS_Readc(struct htx_data *pHTX, struct ruleinfo *pRule,
				struct blk_num_typ *pblk_num);

int init_element(struct htx_data *phtx_info, struct ruleinfo *prule_info,
				int loop, struct blk_num_typ *pblk_num);

int read_status(struct htx_data *phtx_info, struct ruleinfo *prule_info,
				int loop, struct blk_num_typ *pblk_num);

int medium_load(struct htx_data *phtx_info, struct ruleinfo *prule_info,
				int loop, struct blk_num_typ *pblk_num);

int medium_unload(struct htx_data *phtx_info, struct ruleinfo *prule_info,
				int loop, struct blk_num_typ *pblk_num);

int loc_block(struct htx_data *phtx_info, struct ruleinfo *prule_info,
				int loop, struct blk_num_typ *pblk_num);

int read_posit(struct htx_data *phtx_info, struct ruleinfo *prule_info,
				int loop, struct blk_num_typ *pblk_num);

int asearch_file(struct htx_data *phtx_info, struct ruleinfo *prule_info,
				int loop, struct blk_num_typ *pblk_num);

int asearch_rec(struct htx_data *phtx_info, struct ruleinfo *prule_info,
				int loop, struct blk_num_typ *pblk_num);

int write_unload(struct htx_data *phtx_info, struct ruleinfo *prule_info,
				int loop, struct blk_num_typ *pblk_num, char *wbuf);

int twin_tape(struct htx_data *phtx_info, struct ruleinfo *prule_info,
				int loop, struct blk_num_typ *pblk_num);

int twps_tape(struct htx_data *phtx_info, struct ruleinfo *prule_info,
				int loop, struct blk_num_typ *pblk_num);

int twrd_stat(struct htx_data *phtx_info, struct ruleinfo *prule_info,
				int loop, struct blk_num_typ *pblk_num);

int twmv_tape(struct htx_data *phtx_info, struct ruleinfo *prule_info,
				int loop, struct blk_num_typ *pblk_num);

int tape_unload(struct htx_data *phtx_info, struct ruleinfo *prule_info,
				int loop, struct blk_num_typ *pblk_num, char *wbuf);
int unload_write(struct htx_data *phtx_info, struct ruleinfo *prule_info,
				int loop, struct blk_num_typ *pblk_num, char *wbuf);

int cdmv_tape(struct htx_data *phtx_info, struct ruleinfo *prule_info,
				int loop, struct blk_num_typ *pblk_num);

int cdrd_stat(struct htx_data *phtx_info, struct ruleinfo *prule_info,
				int loop, struct blk_num_typ *pblk_num);

int himove(struct htx_data *phtx_info, struct ruleinfo *prule_info,
				int loop, struct blk_num_typ *pblk_num);

int hielem(struct htx_data *phtx_info, struct ruleinfo *prule_info, int loop,
				struct blk_num_typ *pblk_num);

int hiinit(struct htx_data *phtx_info, struct ruleinfo *prule_info, int loop,
				struct blk_num_typ *pblk_num);

int hidal_unload(struct htx_data *phtx_info, struct ruleinfo *prule_info,
				int loop, struct blk_num_typ *pblk_num, char *wbuf);

int do_cmd(struct htx_data *phtx_info, struct ruleinfo *prule_info,
				struct blk_num_typ *pblk_num);

#ifdef __HTX_LINUX__
int get_scsi_devname(char *scsi_dev,char *tapename);
#endif

/*
 * Functions declaration for posix.c
 */
size_t	htx_strlen(const char *string);
char	*htx_strcpy(char *dest, const char *src);
char	*htx_strncpy(char *dest, const char *src, size_t n);
char	*htx_strcat(char *dest, const char *src);
char	*htx_strchr(const char *s, int c);
int	htx_strcmp(const char *s1, const char *s2);
size_t	htx_strspn(const char *s, const char *accept);
size_t	htx_strcspn(const char *s, const char *reject);

/*
 * Function declaration for proc_rule.c
 */
int proc_rule(struct htx_data *phtx_info, struct ruleinfo *prule_info,
				char *wbuf, char *rbuf, struct blk_num_typ *pblk_num);

/*
 * Function declaration for prt_msg.c
 */
void info_msg(struct htx_data *phtx_info, struct ruleinfo *prule_info, int loop,
				struct blk_num_typ *pblk_num, char *msg_text);
void prt_msg(struct htx_data *phtx_info, struct ruleinfo *prule_info, int loop,
				struct blk_num_typ *pblk_num, int err, int sev,char *text);

void prt_msg_asis(struct htx_data *phtx_info, struct ruleinfo *prule_info, int loop,
				struct blk_num_typ *pblk_num, int err, int sev, char *text);

int cmp_buf(struct htx_data *ps, char *wbuf, char *rbuf, size_t len, char *misc_data);
int savebuf_tofile(char *buf, size_t len, char *fname, struct htx_data *ps);

#ifndef __HTX_LINUX__
/*
 * Some libc functions which are not declared in AIX headers.
 */
extern int atoi(char*);
extern int system(const char*);
extern int bzero(char*, int);
extern long time(long*);
extern void free(void*);
extern void *malloc(size_t);
extern void exit(int);
extern char *getenv(char*);
extern long int nrand48(unsigned short int[]);
extern int odm_initialize();
extern struct CuAt *getattr(char*, char*, int, int*);
extern int putattr(struct CuAt*);
extern long atol(char*);
extern long int nrand48(unsigned short int[]);
extern struct tm* localtime(const time_t*);
extern time_t time(time_t*);
#endif
#endif /*_H_HXETAPE_*/
