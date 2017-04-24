/* @(#)44  1.9  src/htx/usr/lpp/htx/inc/htxlibdef.h, htx_libhtx, htx530 10/19/03 23:37:48 */
/* Component = htx_libhtx_lx */

/*
 *  htxlibdef.h -- HTX Library (libhtx.a) Function Declarations
 *
 *    This include file is the respository for all HTX Library
 *    (libhtx.a) function declarations.
 *
 */

#ifndef HTXLIBDEF_H
#define HTXLIBDEF_H

#include <sys/types.h>


#define mtyp_t long

#ifdef C_P

/*
 *  function to compare two buffers
 */
extern "C" {
int hxfcbuf(struct htx_data *, char *, char *, size_t, char *);
}



/*
 *  function to send a message to the HTX system
 */
extern "C" {
int hxfmsg(struct htx_data *, int,  enum sev_code, char *);
}

/*
 *  function to open a hft
 */
extern "C" {
int hxfohft(int);
}

/*
 *  function to read a pattern from a file and copy that pattern to a buffer
 */
extern "C" {
int hxfpat(char *, char *, int);
}

/*
 *  function to save two compare buffers to files in the .../dump
 *  directory
 */
extern "C" {
int hxfsbuf(char *, size_t, char *, struct htx_data *);
}

/*
 *  function to wait on semaphore until HTX startup is completed
 */
extern "C" {
int hxf_startup_complete(struct htx_data *);
}

/*
 *  function to update data for the HTX system
 */
extern "C" {
int hxfupdate_tunsafe(char, struct htx_data *);
}

/*
 *  Internal hxfupdate() function
 */
extern "C" {
void htx_error(struct htx_data *data, char* msg_send);
}

/*
 *  Internal hxfupdate() function
 */
extern "C" {
int htx_finish(struct htx_data *, int *);
}

/*
 *  Internal hxfupdate() function
 */
extern "C" {
int htx_update(struct htx_data *);
}

/*
 *  Internal hxfupdate() function
 */
extern "C" {
int htx_start(struct htx_data *, int *, int *);
}

/*
 *  Internal hxfupdate() function
 */
extern "C" {
int htx_pseudostart(struct htx_data *, int *);
}

/*
 *  Internal hxfupdate() function
 */
extern "C" {
int dupdev_cmp ( int , char * );
}

/*
 *  Internal hxfupdate() function
 */
extern "C" {
int receive_all(int , char *);
}
/*
 *  Internal hxfupdate() function
 */
extern "C" {
int sendp(struct htx_data *, mtyp_t);
}

extern "C" {
int stx_license(void);
}

extern "C" {
int htx_license_key_validate(char *);
}


/*
 * Internal HTX malloc function.
 */
 extern "C" {
void * htx_malloc(size_t size);
}

/*
 * Internal HTX free function.
 *
 */
extern "C" {
void  htx_free(void * ptr);
}

/*
 * Internal HTX localtime function.
 *
 */
extern "C" {
struct tm * htx_localtime(const time_t *p_time);
}

/*
 * Internal HTX ctime function.
 *
 */
extern "C" {
char * htx_ctime(const time_t *p_time);
}

extern "C" {
int htx_message(struct htx_data *data, char* msg_send);
}

#else

/*
 *  function to compare two buffers
 */
int hxfcbuf(struct htx_data *, char *, char *, size_t, char *);



/*
 *  function to send a message to the HTX system
 */
int hxfmsg(struct htx_data *, int,  enum sev_code, char *);

/*
 *  function to open a hft
 */
int hxfohft(int);

/*
 *  function to read a pattern from a file and copy that pattern to a buffer
 */
int hxfpat(char *, char *, int);

/*
 *  function to save two compare buffers to files in the .../dump
 *  directory
 */
int hxfsbuf(char *, size_t, char *, struct htx_data *);

/*
 *  function to wait on semaphore until HTX startup is completed
 */
int hxf_startup_complete(struct htx_data *);

/*
 *  function to update data for the HTX system
 */
int hxfupdate(char, struct htx_data *);

/*
 *  Internal hxfupdate() function
 */
void htx_error(struct htx_data *data, char* msg_send);

/*
 *  Internal hxfupdate() function
 */
int htx_finish(struct htx_data *, int *);

/*
 *  Internal hxfupdate() function
 */
int htx_update(struct htx_data *);

/*
 *  Internal hxfupdate() function
 */
int htx_start(struct htx_data *, int *, int *);

/*
 *  Internal hxfupdate() function
 */
int htx_pseudostart(struct htx_data *, int *);

/*
 *  Internal hxfupdate() function
 */
int dupdev_cmp ( int , char * );

/*
 *  Internal hxfupdate() function
 */
int receive_all(int , char *);
/*
 *  Internal hxfupdate() function
 */
int sendp(struct htx_data *, mtyp_t);

int stx_license(void);

int htx_license_key_validate(char *);

/*
 * Internal HTX malloc function.
 */
void * htx_malloc(size_t size);

/*
 * Internal HTX free function.
 */
void  htx_free(void * ptr);

/*
 * Internal HTX localtime function.
 */
struct tm * htx_localtime(const time_t *p_time);

/*
 * Internal HTX ctime function.
 */
char * htx_ctime(const time_t *p_time);

int htx_message(struct htx_data *data, char* msg_send);

int htx_get_msg(struct htx_data *data, char *msg_send);

struct tm *htx_localtime_r (const time_t *timep, struct tm *result);

#endif

static const char IBM_copyright_string[]="\n\
	Licensed Materials - Property of IBM\n\
	(C) Copyright IBM Corp. 2010, 2013,  All rights reserved.\n\
\n\
	US Government Users Restricted Rights - Use, duplication or\n\
	disclosure restricted by GSA ADP Schedule Contract with IBM Corp.\n";

#endif  /* HTXLIBDEF_H */
