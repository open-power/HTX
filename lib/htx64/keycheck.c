/* IBM_PROLOG_BEGIN_TAG */
/*
 * Copyright 2003,2016 IBM International Business Machines Corp.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *       http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or
 * implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
/* IBM_PROLOG_END_TAG */

/* @(#)12	1.7  src/htx/usr/lpp/htx/lib/htx64/keycheck.c, htx_license, htxubuntu 7/15/16 07:06:20 */

#include <expirekey.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <errno.h>
#include <ctype.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>

#define GOOD 0
#define EXPIRED 2
#define FAIL -1
#define NULL 0
#define TRUE 1
#define STRING_LENGTH 59
#define ACTIVATION_KEY_LENGTH 59
#define DATE_TIME_SIZE 20
#define STXAPP_KEY "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmno"

#define KEY_DATA_LENGTH    512
#define KEY_DATE_SIZE      5
#define NO_KEY_FILE        66
#define OLD_VERSION        67

#define VER_OFFSET_LOCATION 		3
#define MIN_VER_OFFSET 			5
#define MAX_VER_OFFSET 			16
#define HOST_OFFSET_LOCATION  		4
#define MIN_HOST_OFFSET 		109 /*125*/
#define MAX_HOST_OFFSET 		110 /*127*/
#define PREV_DATE_OFFSET_LOCATION 	73
#define MIN_PREV_DATE_OFFSET 		78
#define MAX_PREV_DATE_OFFSET 		85
#define EXP_DATE_OFFSET_LOCATION 	124
#define MIN_EXP_DATE_OFFSET  		128
#define MAX_EXP_DATE_OFFSET  		135
#define REQ_DATE_OFFSET_LOCATION	141
#define MIN_REQ_DATE_OFFSET		143
#define MAX_REQ_DATE_OFFSET		152
#define EMAIL_LENGTH_OFFSET_LOCATION    90
#define MIN_EMAIL_LENGTH_OFFSET		93
#define MAX_EMAIL_LENGTH_OFFSET		96
#define EMAIL_OFFSET_LOCATION		170 /*43*/
#define MIN_EMAIL_OFFSET		175 /*59*/
#define MAX_EMAIL_OFFSET		180 /*67*/
#define RELEASES_OFFSET_LOCATION	119
#define MIN_RELEASES_OFFSET		190
#define MAX_RELEASES_OFFSET		191
#define NUMR_OFFSET_LOCATION		230
#define MIN_NUMR_OFFSET			240
#define MAX_NUMR_OFFSET			255

static long int Vers, Vers_key;
static char Emailid[12] = {'\0'}, Releases[37] = {'\0'}, NumR[2] = {'\0'}, ReqDate[6] = {'\0'}, EmailLen[3] = {'\0'}, Version[6] = {'\0'} ;
static int nUMr = 0, eMAILlEN = 0, vERSION;
static int req_year, req_month, req_day ;
static int verchk, keyfilefound = 1 ;


int TestKeyFile(char *filename, char *version, int *exp_year, int *exp_month, int *exp_day, char *release, char exp_host);
int Activate(char *version, int *exp_year, int *exp_month, int *exp_day, char *release, char *activation_key);
int InitialActivate(char *version, int *exp_year, int *exp_month, int *exp_day, char *release);
int InvalidateKeyFile(char *filename);
int UpdateKeyFile(char *filename, char *version, int exp_year, int exp_month, int exp_day,char *releases, char host_name);
int DecodeKeyData(unsigned char *key_data, char *version, int *prev_year, int *prev_month, int *prev_day, int *prev_hour, int *prev_minute, int *prev_second, int *exp_year, int *exp_month, int *exp_day, char *releases, char *host_name);
void EncodeKeyData(unsigned char *key_data, char *version,
                   int cur_year, int cur_month,  int cur_day,
                   int cur_hour, int cur_minute, int cur_second,
                   int exp_year, int exp_month,  int exp_day, char *releases, char host_name);
int  WriteKeyFile(char *filename, unsigned char *key_data);
int  ReadKeyFile(char *filename, unsigned char *key_data);
void  TimeStamp(char *, int);           /* Get current time stamp */
unsigned int Random(int seed, int min, int  max);
int DecodeKey(unsigned char *key);

char* itoa(int i, char *ch)
{

sprintf(ch, "%d", i);
return ch;

}

char num2alpha(int n) {
 char c = n + 97 - 1 ;
 return c ;
}

int alpha2num (char e) {
 int n;

 if(e == '@') return 64;
 if(isdigit(e)) return e;

 /*n = e - '0';
 printf("In alpha2num, n = %d\n",n);*/

 if( e >= 97 && e <= 122 ) /*lower case alphabets*/
   return ( e - 97 ) ;
 else if ( e >= 65 && e <= 90 )  /* upper case alphabets */
   return ( e - 65 ) ;
 return -1;
}


int map_char (char c, int index)
 {

  int i, j;

  for( i = 0 ; i < strlen(map[index]) ; i++ )
   if ( map[index][i] == c )
     return i;

  printf("\n Aborting because of invalid key data, c = %c, index = %d\n", c, index);
  exit(-2);

 }


/****************************************************************************/
/*                                                                          */
/* Function: ProgramExpire                                                  */
/*                                                                          */
/* Description: read a special key file, if it fails, or is expired, then   */
/*              request an activation key. If either one works, update the  */
/*              key file. If all works OK, program can continue.            */
/*                                                                          */
/* Input: version                                                           */
/*                                                                          */
/* Output: return code (GOOD or FAIL)                                       */
/*                                                                          */
/****************************************************************************/

/*int ProgramExpire(char *filename, char *version,
                  char *host_name, char *activation_key)*/
int stx_license()
{
  char month[3], day[3], year[5];
  int file_rc = FAIL;
  int rc = FAIL;
  int exp_year  = -1;
  int exp_month = -1;
  int exp_day   = -1;
  char activation_key[60]={'\0'};
  char buf[60] = {'\0'};
  FILE *fp;
  char a_char[2];
  char *filename; /* ="/usr/lpp/htx/etc/scripts/htxkey";*/
  char buffer[35];
  char temp[15],temp1[35];
  char version[6];
  char host_name[3];
  int i,j;
  char release[7];
  char *test,*p;
  char key_filename[100];
  int fd;
  ssize_t read_rc;
  struct stat stat_buf;

  srand(time(0));
  fflush(stdout);

  host_name[2] = '\0';
  version[5] = '\0';


  filename = (char *)htx_malloc(sizeof(char) * 150);
  if(filename == NULL)
  {
	printf("Unable to malloc\n");
	exit(-1);
  }


  if(!(getenv("HTXSCRIPTS")))
  {
  	strcpy(filename,"/usr/lpp/htx/etc/scripts/htxkey");
  }

  else
  {
 	strcpy(filename,getenv("HTXSCRIPTS"));
	strcat(filename,"/htxkey");
  }

  a_char[1] = '\0';

  fflush(stdout);

#ifndef __HTX_LINUX__
 fp = fopen("/usr/lpp/htx/htxaixlevel","r");

 if(fp == NULL) {
	printf("Unable to open /usr/lpp/htx/htxaixlevel, errno = %d\n",errno);
	exit(-1);
  }

        fscanf(fp,"%s",buffer);
        fscanf(fp,"%s",buffer);
        for(i=7;buffer[i]!=')';i++)
                temp[i - 7] = buffer[i];
        temp[i - 7] = '\0';
	fflush(stdout);

        if(strlen(temp) == 3)
        {
                version[0]='0';
                version[1] = temp[0];
                version[2] = '.';
                version[3] = temp[1];
                version[4] = temp[2]; version[5] = '\0';
        }
        else
        {
                version[0] = temp[0];
                version[1] = temp[1];
                version[2] = '.';
                version[3] = temp[2]; version[4] = temp[3]; version[5] = '\0';
        }

        fclose(fp);

   fp = popen("hostname","r");
   if(fp == NULL) {
	printf("Error in popen while executing hostname, errno = %d\n",errno);
	exit(-1);
   }

        fgets(buffer,35,fp);
	host_name[0] = buffer[0] ;
	host_name[1] = '\0';
        pclose(fp);

     fp = popen(" what /usr/lpp/htx/bin/hxssup | awk '{print $5}' | grep htx | sort -u | head -n1","r");
      if(fp == NULL) {
        printf("Error in popen while executing what /usr/lpp/htx/bin/hxssup..., errno = %d\n",errno);
        exit(-1);
      }

    fgets(buffer,7,fp);
        pclose(fp);

        strcpy(temp,buffer);

        p = strstr(buffer,"htx5");
        release[0] = p[3]; release[1] = p[4]; release[2] = p[5]; release[3] = '\0';
  	fflush(stdout);

#else

	fp = popen("[ -f /usr/lpp/htx/etc/version ] && cat /usr/lpp/htx/etc/version || rpm -qa | grep htx","r");
	if(fp == NULL)
	{
		printf("Unable to get HTX Release name\n");
		exit(-1);
	}

	fgets(buffer,16,fp);
	pclose(fp);

	release[0] = buffer[3]; release[1] = buffer[4]; release[2] = buffer[5]; release[3] = buffer[6]; release[4] = buffer[7];
	release[5] = buffer[8]; release[6] = '\0';

	version[0] = '0'; version[1] = buffer[10]; version[2] = '.'; version[3] = buffer[11]; version[4] = buffer[12];
	version[5] = '\0';

  	fflush(stdout);

#endif


  fflush(stdout);

  /*------------------------------------------------------------------------*/
  /* Test Key file contents                                                 */
  /*------------------------------------------------------------------------*/
  rc = TestKeyFile(filename, version, &exp_year, &exp_month, &exp_day,
                                                       release, *host_name);

  /*------------------------------------------------------------------------*/
  /* If key file exists, but the version is wrong, try initial activation   */
  /*------------------------------------------------------------------------*/
  /*srand(0);*/
  switch(rc)
  {
    case OLD_VERSION:             /* an old key file from a prior version   */
      rc = InitialActivate(version, &exp_year, &exp_month, &exp_day, release);
      break;
    case NO_KEY_FILE:
      /*rc = FAIL;*/
      keyfilefound = 0 ;
      break;
    default:
      if (activation_key[0] != '\0')
        printf("Activation key ignored - not needed. \n");
        /*printf("Activation key %s ignored - not needed. \n", activation_key);*/
      break;
  }


  /*------------------------------------------------------------------------*/
  /* If key file validation failed, request an activation key.              */
  /*------------------------------------------------------------------------*/
  if (rc == NO_KEY_FILE || rc == EXPIRED || rc ==FAIL) {
                         /* key file not there, or invalid */

	if(rc == EXPIRED)
		printf("\n The key file has expired ");

	fflush(stdout);

	strcpy(key_filename, " ");
	if (rc == NO_KEY_FILE) {
		if ((stat("/tmp/tempkey", &stat_buf) == 0 ) && (S_ISREG(stat_buf.st_mode))) {
			strcpy(key_filename, "/tmp/tempkey");
		}
	}

	if ( strcmp (key_filename, " ") == 0) {
		printf("\nCould not find the default license key /tmp/tempkey.\nPlease enter the path of the key file along with the filename:\n");
		fflush(stdout);
		scanf(" %s",key_filename);
	}

	fd = open(key_filename,O_RDONLY);
	if(fd == -1)
	{
		printf("\n Unable to open %s. Errno = %d\n", key_filename,errno);
	        exit(-1);
        }


	read_rc = read(fd,buf,KEYLEN);
	if(read_rc != KEYLEN)
	{
		printf("read_rc = %zd\n",read_rc);
		printf("\n Did not read the key file properly\n");
		exit(-3);
	}

	close(fd);


          strncpy(activation_key,buf, KEYLEN); 

	DecodeKey(activation_key);

        rc = Activate(version, &exp_year, &exp_month, &exp_day, release, activation_key);
    }


  /*------------------------------------------------------------------------*/
  /* If key file good, or activation key good, update key file.             */
  /*------------------------------------------------------------------------*/
  if (rc == GOOD)
  {

    if(verchk == 0 && keyfilefound == 0)
	    {
		file_rc = UpdateKeyFile(filename, version, exp_year, exp_month, exp_day,
                                                Releases, *host_name);
	    }
    else
  	{
	    file_rc = UpdateKeyFile(filename, Version, exp_year, exp_month, exp_day,
                                                Releases, *host_name);
	}

    if (file_rc == FAIL)               /* if update failed, reflect that to */
      rc = FAIL;                       /* the caller.                       */

    *month = exp_month;                /* set in callers variables          */
    *day = exp_day;
    *year = exp_year;
  }

  htx_free(filename);


  return(rc);
}


/****************************************************************************/
/*                                                                          */
/* Function: TestKeyFile                                                    */
/*                                                                          */
/* Description: read a special key file, and compares values with program   */
/*              expiration. If the file is not available, or the program    */
/*              has expired, the function returns a FALSE                   */
/*                                                                          */
/* Input: none                                                              */
/*                                                                          */
/* Output: boolean expired.                                                 */
/*                                                                          */
/****************************************************************************/
int TestKeyFile(char *filename, char *version,
    int *exp_year, int *exp_month, int *exp_day, char *release, char exp_host)
{
  int rc = FAIL;                                /* return to caller         */
  unsigned char key_data[KEY_DATA_LENGTH];      /* data area for file read. */
  int i,j;
  int year;
  int month;
  int day;
  int hour;
  int minute;
  char host_name;

  char key_version[11];
  char key_emailid[12] ;
  char key_releases[36] ;
  char rel[4];

  int prev_year;
  int prev_month;
  int prev_day;
  int prev_hour;
  int prev_minute;
  int prev_second;
  char time_stamp[DATE_TIME_SIZE];
  char *q;
  int other[7];
  char *buf1 = (char *)htx_malloc(sizeof(char) * 10);
  /*char *buf2 = (char *)malloc(sizeof(char) * 10);*/

  TimeStamp(time_stamp,DATE_TIME_SIZE);           /* Get current time stamp */
  month  = strtol(time_stamp,    NULL, 10);
  day    = strtol(time_stamp+3,  NULL, 10);
  year   = strtol(time_stamp+6,  NULL, 10);
  hour   = strtol(time_stamp+11, NULL, 10);
  minute = strtol(time_stamp+14, NULL, 10);
  /*year = year - 2000;*/
  /*-------------------------------------------------------------------------*/
  /* See if a key file exists. If not, set the return code and bail out.     */
  /*-------------------------------------------------------------------------*/

  rc =  ReadKeyFile(filename, key_data);

  if (rc == GOOD)
  {
    /*-----------------------------------------------------------------------*/
    /* Decode the data read from the key file.                               */
    /*-----------------------------------------------------------------------*/
    rc = DecodeKeyData(key_data, key_version,
                       &prev_year, &prev_month,  &prev_day,
                       &prev_hour, &prev_minute, &prev_second,
                       exp_year,  exp_month, exp_day, key_releases, &host_name);
    /*-----------------------------------------------------------------------*/
    /* Check for invalid values in the date fields.                          */
    /*-----------------------------------------------------------------------*/

    if (rc == GOOD)
    {
      if ((prev_year   > 62   || prev_year   < 0) ||
          (prev_month  > 12   || prev_month  < 0) ||
          (prev_day    > 31   || prev_day    < 0) ||
          (prev_hour   > 24   || prev_hour   < 0) ||
          (prev_minute > 60   || prev_minute < 0) ||
          (prev_second > 60   || prev_second < 0) ||
          (*exp_year   > 62   || *exp_year   < 0) ||
          (*exp_month  > 12   || *exp_month  < 0) ||
          (*exp_day    > 31   || *exp_day    < 0)) {
        printf("Invalid as time is incorrect\n");
        rc = FAIL;
      }
    }
    /*-----------------------------------------------------------------------*/
    /* Compare previous date and time to ensure that it is less than current */
    /*-----------------------------------------------------------------------*/
    if (rc == GOOD)
    {
      if (prev_year > year  ||
          (prev_year == year && prev_month > month) ||
          (prev_year == year && prev_month == month && prev_day > day) ||
          (prev_year == year && prev_month == month && prev_day == day &&
                                                        prev_hour > hour) ||
          (prev_year == year && prev_month == month && prev_day == day &&
                                                     prev_hour == hour &&
                                                     prev_minute > minute))
         /*------------------------------------------------------------------*/
         /* NOTE: seconds checking was removed. in case of network time skew */
         /*------------------------------------------------------------------*/
      {
        rc = FAIL;
      }
    }
    if (rc == FAIL)                                   /* invalid data found. */
      InvalidateKeyFile(filename);

    /*----------------------------------------------------------------------*/
    /* Compare if the release name passed to the stx_license function is one*/
    /* one of those for which the key was originally requested              */

    if(rc == GOOD)
    {


      if(!(strstr(key_releases,"all"))) {
	     verchk = 1 ;
	     rc = FAIL ;
      }
      if( rc == FAIL && (strstr(key_releases,release)) != NULL)
	    rc = GOOD ;

/*      if(rc == GOOD)
 	printf("\n Key valid \n");
      else
	printf("\n Invalid Key \n"); */

    }

    /*-----------------------------------------------------------------------*/
    /* Compare extracted version with the current version                    */
    /*-----------------------------------------------------------------------*/
    if (rc == GOOD && verchk)
	{
	other[0] = key_version[0];
	other[1] = key_version[1];
	other[2] = key_version[2];
	other[3] = key_version[3];
	other[4] = key_version[4];
	other[5] = '\0';

      if (strncmp(key_version, version, 5)){
	for(i =0 ; i < 10 ; i++)
		buf1[i] = '\0' ;

	for(i = 0 ; i < 2 ; i++)
		buf1[i] = version[i];

	buf1[2] = '\0' ;

	Vers = strtol(buf1,NULL,10) * 100;

	for(i = 3 ; i < 5 ; i++) {
                buf1[i - 3] = version[i];
	}
	buf1[2] = '\0' ;


	Vers += strtol(buf1,NULL,10) ;


	for(i =0 ; i < 10 ; i++)
                buf1[i] = '\0' ;

        for(i = 0 ; i < 2 ; i++)
                buf1[i] = key_version[i];

        buf1[2] = '\0' ;

        Vers_key = strtol(buf1,NULL,10) * 100;

        for(i = 3 ; i < 5 ; i++) {
                buf1[i - 3] = key_version[i];
        }
        buf1[2] = '\0' ;


        Vers_key += strtol(buf1,NULL,10) ;

	if( (Vers - Vers_key) > 25 || (Vers - Vers_key) < -25 )
        	rc = FAIL;
	}

    }

    sprintf(Version,"%s",key_version);

    /*-----------------------------------------------------------------------*/
    /* Compare expiration date to insure that it is greater than current     */
    /*-----------------------------------------------------------------------*/
    if (rc == GOOD)
    {
      if ((year  > *exp_year)                          ||
          (year == *exp_year && month > *exp_month)    ||
          (year == *exp_year && month == *exp_month && day >= *exp_day))
      {
        /*BlastNewVersion();*/
        rc = EXPIRED;

      }
    }
    /*-----------------------------------------------------------------------*/
    /* Compare host name to insure it is proper. NOTE: Only one character    */
    /* is being tested at this time. Later, this test should be moved        */
    /* before the test for expired. This should be done when versions        */
    /* prior to 5.01 have all expired.                                       */
    /*-----------------------------------------------------------------------*/
    /*if (rc == GOOD)
    {
      if (host_name != exp_host)
        rc = FAIL;
    }
    */


  }
  else
    rc = NO_KEY_FILE;

  htx_free(buf1);

  return (rc);
}

/****************************************************************************/
/*                                                                          */
/* Function: UpdateKeyFile                                                  */
/*                                                                          */
/* Description: update the key file with the encoded version, current date  */
/*              and time, and the expiration date and time.                 */
/*                                                                          */
/* Input: none                                                              */
/*                                                                          */
/* Output: boolean expired.                                                 */
/*                                                                          */
/****************************************************************************/
int UpdateKeyFile(char *filename, char *version,
     int exp_year, int exp_month, int exp_day, char *releases, char host_name)
{
  int   rc;                                     /* return to caller         */
  unsigned char key_data[KEY_DATA_LENGTH + 1];      /* data area for file read. */
  char  time_stamp[DATE_TIME_SIZE];
  int year;
  int month;
  int day;
  int hour;
  int minute;
  int second;

  TimeStamp(time_stamp,DATE_TIME_SIZE);           /* Get current time stamp */
  month  = strtol(time_stamp,    NULL, 10);
  day    = strtol(time_stamp+3,  NULL, 10);
  year   = strtol(time_stamp+6,  NULL, 10);
  hour   = strtol(time_stamp+11, NULL, 10);
  minute = strtol(time_stamp+14, NULL, 10);
  second = strtol(time_stamp+17, NULL, 10);
  /*year = year - 2000;*/


  /*-----------------------------------------------------------------------*/
  /* Encode the date and time and write the key file                       */
  /*-----------------------------------------------------------------------*/

  key_data[512] = '\0';
  EncodeKeyData(key_data, version,
                   year, month,  day, hour, minute, second,
                   exp_year, exp_month, exp_day, releases, host_name);

  rc = WriteKeyFile(filename, key_data);

  return (rc);
}

/****************************************************************************/
/*                                                                          */
/* Function: InvalidateKeyFile                                              */
/*                                                                          */
/* Description: update the key file with invalid data.                      */
/*                                                                          */
/* Input: none                                                              */
/*                                                                          */
/* Output: boolean expired.                                                 */
/*                                                                          */
/****************************************************************************/
int InvalidateKeyFile(char *filename)
{
  int   rc;                                     /* return to caller         */
  unsigned char key_data[KEY_DATA_LENGTH];      /* data area for file read. */

  memset(key_data, '1', KEY_DATA_LENGTH);          /* clear the structure     */
  /*-----------------------------------------------------------------------*/
  /* Encode the date and time and write the key file                       */
  /*-----------------------------------------------------------------------*/


  rc = WriteKeyFile(filename, key_data);


  return (rc);
}

/****************************************************************************/
/*                                                                          */
/* Function: DecodeKeyData                                                  */
/*                                                                          */
/* Description: after reading the key file, this routine is called to       */
/*              extract the information, and decode it.   .                 */
/*                                                                          */
/* Input: data from file, and variables to receive values.                  */
/*                                                                          */
/* Output: none                                                             */
/*                                                                          */
/****************************************************************************/
int  DecodeKeyData(unsigned char *key_data, char *version,
                   int *prev_year, int *prev_month, int *prev_day,
                   int *prev_hour, int *prev_minute, int *prev_second,
                   int *exp_year, int *exp_month, int *exp_day, char *releases,
							     char *host_name)
{
  int rc = GOOD;
  int i, j, k;
  char *a_char;
  char a_num[4];
  int digit;
  char digit_char[3];
  char exp_date_key[7];
  char prev_date_key[7];
  char req_date_key[7];
  char email_key[12];
  char vers_key[5];
  char numr_key[2];
  char release_key[37];
  char temp[12];
  char emlen_key[2];

  unsigned int ver_offset = key_data[VER_OFFSET_LOCATION];
  unsigned int host_offset = key_data[HOST_OFFSET_LOCATION];
  unsigned int prev_offset = key_data[PREV_DATE_OFFSET_LOCATION];
  unsigned int exp_offset = key_data[EXP_DATE_OFFSET_LOCATION];
  unsigned int req_offset = key_data[REQ_DATE_OFFSET_LOCATION];
  unsigned int numr_offset= key_data[NUMR_OFFSET_LOCATION];
  unsigned int email_offset=key_data[EMAIL_OFFSET_LOCATION];
  unsigned int rel_offset = key_data[RELEASES_OFFSET_LOCATION];
  unsigned int elen_offset= key_data[EMAIL_LENGTH_OFFSET_LOCATION];
  a_num[1] = 0;
  a_num[2] = 0;

  /*printf("key_data[exp_offset + 0] = %c, key_data[exp_offset + 1] = %c, key_data[exp_offset + 2] = %c \n", key_data[exp_offset + 0], key_data[exp_offset + 1], key_data[exp_offset + 2]); */

    /*-----------------------------------------------------------------------*/
    /* Extract the key values from the input data.                           */
    /*-----------------------------------------------------------------------*/
    for (i = 0; i < 4; i++)
      vers_key[i] = key_data[ver_offset + i];
    for (i = 0; i < 6; i++)
      prev_date_key[i] =  key_data[prev_offset + i];
    for (i = 0; i < 3; i++)
      exp_date_key[i] = key_data[exp_offset + i];
    for (i = 0; i < 3; i++)
      req_date_key[i] = key_data[req_offset + i];


    vers_key[4] = '\0';
    prev_date_key[6] = '\0';
    exp_date_key[3] = '\0';
    req_date_key[3] = '\0';


	/*printf("In DecodeKeyData, exp_offset = %d\n", exp_offset);
	printf("In DecodeKeyData, after extracting the key values, prev_date_key = %s\n", prev_date_key);
	printf("In DecodeKeyData, after extracting the key values, exp_date_key[0] = %c, exp_date_key[1] = %c, exp_date_key[2] = %c\n", exp_date_key[0], exp_date_key[1], exp_date_key[2]);
	printf("In DecodeKeyData, after extracting the key values, req_date_key = %s\n", req_date_key); */

    /* Decoding the emailid length */
    emlen_key[0] = key_data[elen_offset];

        digit = map_char(emlen_key[0],0);
        eMAILlEN = digit ;
        sprintf(digit_char, "%1d", digit);
        strcpy(EmailLen,digit_char);

     EmailLen[1] = '\0';

    for (i = 0; i < eMAILlEN; i++)
      email_key[i] = key_data[email_offset + i];

    numr_key[0] = key_data[numr_offset];

        digit = map_char(numr_key[0],22);
	nUMr = digit ;
	sprintf(digit_char, "%1d", digit);
	strcpy(NumR,digit_char);

     NumR[1] = '\0';

    for (i = 0; i < 36; i++)
      release_key[i] = key_data[rel_offset + i];
    release_key[36] = '\0';

    /*-----------------------------------------------------------------------*/
    /* Extract the first character of the hostname                           */
    /*-----------------------------------------------------------------------*/
    *host_name = key_data[host_offset];

    /*-----------------------------------------------------------------------*/
    /* decode the version from the extracted key                             */
    /*-----------------------------------------------------------------------*/

    digit = map_char(vers_key[0],17);
    vERSION = digit * 1000;
    sprintf(digit_char, "%1d", digit);

      version[0] = digit_char[0];

    digit = map_char(vers_key[1],18);
    vERSION = vERSION + (digit * 100);
    sprintf(digit_char, "%1d", digit);
    version[1] = digit_char[0];

    version[2] = '.';

    digit = map_char(vers_key[2], 20);
    vERSION = vERSION + (digit * 10);
    sprintf(digit_char, "%1d", digit);
    version[3] = digit_char[0];

    if (vers_key[3] == '_')
      version[4] = ' ';
    else
    {
      digit = map_char(vers_key[3],21);
      vERSION = vERSION + digit;
      sprintf(digit_char, "%1d", digit);
      version[4] = digit_char[0];
    }
    version[5] = '\0';

    /*-----------------------------------------------------------------------*/
    /* decode the e-mail id						     */
    /*-----------------------------------------------------------------------*/
      for (i = 0, j = 6; i < eMAILlEN ; i++,j++)
	{

 		if(email_key[i] == '@')
			Emailid[i] = '@' ;
		else {
			k = map_char(email_key[i],j) + 1;
			Emailid[i] = num2alpha(k);
		}
	}

	Emailid[eMAILlEN] = '\0';

      /*printf("In DecodeKeyData, Emailid = %s\n",Emailid); */

    /*-----------------------------------------------------------------------*/
    /* decode the releases                                                   */
    /*-----------------------------------------------------------------------*/
	for(i = 0; i < 36; i++)
        	Releases[i] = '^';

	for(i=0;i < 36; i+=6)
	{
		if(release_key[i] == '^')
			Releases[i] = '^' ;
		else
		{
		        j = map_char(release_key[i], 53);
		        Releases[i] = (char)(j + 48);
		}
	}

	for(i=1;i < 36; i+=6)
        {
                if(release_key[i] == '^')
                        Releases[i] = '^' ;
                else
                {
                        j = map_char(release_key[i], 54);
                        Releases[i] = (char)(j + 48);
                }
        }

	for(i=2;i < 36; i+=6)
        {
                if(release_key[i] == '^')
                        Releases[i] = '^' ;
                else
                {
                        j = map_char(release_key[i], 53);
                        Releases[i] = (char)(j + 48);
                }
        }

	for(i=3,k=0;i < 36; i+=6,k+=5)
        {
                if(release_key[i] == '^')
                        Releases[i] = '^' ;
                else
                {
                        j = map_char(release_key[i], k + 25) ;

                        Releases[i] = (char)(j + 48);
                }
        }

	for(i=4,k=0;i < 36; i+=6,k+=5)
        {
                if(release_key[i] == '^')
                        Releases[i] = '^' ;
                else
                {
                        j = map_char(release_key[i], k + 26);
                        Releases[i] = (char)(j + 48);
                }
        }

	for(i=5,k=0;i < 36; i+=6,k+=5)
        {
                if(release_key[i] == '^')
                        Releases[i] = '^' ;
                else
                {
                        j = map_char(release_key[i], k + 27) ;
                        Releases[i] = (char)(j + 48);
                }
        }

     for(i = 0; i < 36; i++)
	releases[i] = Releases[i];

     /*printf("In DecodeKeyData, Releases = %s\n",Releases); */

    /*-----------------------------------------------------------------------*/
    /* decode the previous run date                                          */
    /*-----------------------------------------------------------------------*/
    *prev_year = map_char(prev_date_key[0], 0);			/* Get the Year */

    *prev_month = map_char(prev_date_key[1], 1); 		/* Get the month      */

    *prev_day = map_char(prev_date_key[2],2);			/* Get the day        */

    *prev_hour = map_char(prev_date_key[3], 3); 		/* Get the hour       */

    *prev_minute = map_char(prev_date_key[4],0); 		/* Get the minute     */

    *prev_second = map_char(prev_date_key[5], 0);		/* Get the second     */

    /*printf("In DecodeKeyData, done with decoding previous run date, prev_year = %d, prev_month= %d, prev_day = %d\n", *(prev_year), *(prev_month), *(prev_day)); */

    /*-----------------------------------------------------------------------*/
    /* decode the expiration date                                            */
    /*-----------------------------------------------------------------------*/
    /*printf("exp_date_key[0] = %c\n",exp_date_key[0]);*/
    *exp_year = map_char(exp_date_key[0], 3); 			/* Get the year value */
    /*printf("*exp_year = %d\n", *exp_year);*/

    /*printf("exp_date_key[1] = %c\n",exp_date_key[1]);*/
    *exp_month = map_char(exp_date_key[1], 4);			/* Get the month      */
    (*exp_month)++;
    /*printf("*exp_year = %d\n", *(exp_month)++);*/

    /*printf("exp_date_key[2] = %c\n",exp_date_key[2]);*/
    *exp_day = map_char(exp_date_key[2], 5);			/* Get the day        */
    (*exp_day)++;
    /*printf("*exp_year = %d\n", *(exp_day)++);*/

    /*printf("In DecodeKeyData, done with decoding expiration date\n");*/

    /*-----------------------------------------------------------------------*/
    /* decode the request date                                            */
    /*-----------------------------------------------------------------------*/
    req_year = map_char(req_date_key[0], 0);			/* Get the year value */

    req_month = map_char(req_date_key[1], 1);			/* Get the month      */
    req_month++;

    req_day = map_char(req_date_key[2], 2);			/* Get the day        */
    req_day++;

    /*printf("In DecodeKeyData, done with decoding request date\n");*/

  return (rc);
}


/****************************************************************************/
/*                                                                          */
/* Function: EncodeKeyData                                                  */
/*                                                                          */
/* Description: take the vesion, current date, and expiration date, and     */
/*              encode them into the key_data cahracter array, which will   */
/*              be written to a key file.                                   */
/*                                                                          */
/* Input: data from file, and variables to receive values.                  */
/*                                                                          */
/* Output: none                                                             */
/*                                                                          */
/****************************************************************************/
void EncodeKeyData(unsigned char *key_data, char *version,
                   int cur_year, int cur_month,  int cur_day,
                   int cur_hour, int cur_minute, int cur_second,
    int exp_year, int exp_month,  int exp_day, char *releases, char host_name)
{
  int i,j,k;
  unsigned char a_char[2] = {0,0};
  char exp_date_key[6];
  char cur_date_key[6];
  char vers_key[4];
  char email_key[12];
  char release_key[36];
  char numr_key[2];
  char req_date_key[7];
  int temp[12];
  char a_num[3];
  char emlen_key[2];

  unsigned int ver_offset     = Random(NULL, MIN_VER_OFFSET, MAX_VER_OFFSET);
  unsigned int prev_offset    = Random(NULL, MIN_PREV_DATE_OFFSET, MAX_PREV_DATE_OFFSET);
  unsigned int exp_offset     = Random(NULL, MIN_EXP_DATE_OFFSET, MAX_EXP_DATE_OFFSET);
  unsigned int host_offset    = Random(NULL, MIN_HOST_OFFSET, MAX_HOST_OFFSET);
  unsigned int email_offset   = Random(NULL, MIN_EMAIL_OFFSET, MAX_EMAIL_OFFSET);
  unsigned int release_offset = Random(NULL, MIN_RELEASES_OFFSET, MAX_RELEASES_OFFSET);
  unsigned int nr_offset      = Random(NULL, MIN_NUMR_OFFSET, MAX_NUMR_OFFSET);
  unsigned int req_offset     = Random(NULL, MIN_REQ_DATE_OFFSET, MAX_REQ_DATE_OFFSET);
  unsigned int elen_offset    = Random(NULL, MIN_EMAIL_LENGTH_OFFSET, MAX_EMAIL_LENGTH_OFFSET);
  a_num[2] = '\0';
  a_num[3] = '\0';

  memset((char *)key_data, 0, KEY_DATA_LENGTH);  /* clear the structure     */

  /*------------------------------------------------------------------------*/
  /* Put data into structure.                                               */
  /*------------------------------------------------------------------------*/
  for (i = 0; i < KEY_DATA_LENGTH; i++) {          /* fill with random data.  */
    /*srand(i);*/
	    key_data[i] = (unsigned char)Random(NULL, 1, 256) ;
  }
  key_data[KEY_DATA_LENGTH] = '\0';
  key_data[VER_OFFSET_LOCATION]   = (unsigned char) ver_offset;
  key_data[PREV_DATE_OFFSET_LOCATION]  = (unsigned char) prev_offset;
  key_data[EXP_DATE_OFFSET_LOCATION] = (unsigned char) exp_offset;
  key_data[HOST_OFFSET_LOCATION] = host_offset;
  key_data[EMAIL_OFFSET_LOCATION] = email_offset;
  key_data[RELEASES_OFFSET_LOCATION] = release_offset;
  key_data[REQ_DATE_OFFSET_LOCATION] = req_offset ;
  key_data[NUMR_OFFSET_LOCATION] = nr_offset ;
  key_data[EMAIL_LENGTH_OFFSET_LOCATION] = elen_offset ;


  if(host_name == '\0')
	  key_data[host_offset] = 'x';
  else
	  key_data[host_offset] = host_name;

  exp_date_key[0] = map[3][exp_year];
  exp_date_key[1] = map[4][exp_month-1];
  exp_date_key[2] = map[5][exp_day-1];


  cur_date_key[0] = map[0][cur_year];
  cur_date_key[1] = map[1][cur_month-1];
  cur_date_key[2] = map[2][cur_day-1];
  cur_date_key[3] = map[3][cur_hour];
  cur_date_key[4] = map[0][cur_minute];
  cur_date_key[5] = map[0][cur_second];

  req_date_key[0] = map[0][req_year];
  req_date_key[1] = map[1][req_month - 1];
  req_date_key[2] = map[2][req_day - 1];

  req_date_key[3] = '\0';

  a_char[0] = version[0];
  vers_key[0] = map[17][atoi((char *)a_char)];
  a_char[0] = version[1];
  vers_key[1] = map[18][atoi((char *)a_char)];
  a_char[0] = version[3];
  vers_key[2] = map[20][atoi((char *)a_char)];
  a_char[0] = version[4];
  vers_key[3] = map[21][atoi((char *)a_char)];

  /*e-mail */
  emlen_key[0] = map[0][eMAILlEN];

  for ( i = 0; i < eMAILlEN ; i++ ) {
        temp[i] = alpha2num(Emailid[i]);
	if(temp[i] == '@')
		email_key[i] = '@' ;
 	else
		email_key[i] = map[i + 6][temp[i]];
   }

  /*numr*/

   numr_key[0] = map[22][nUMr];
   numr_key[1] = '\0';

  /*releases*/

  for(i = 0; i < 36 ; i++)
	release_key[i] = '^' ;

  k = 25;

  for (i = 0; i < 36 ; i += 6 ) /*up to Numr*/
    {
	if(Releases[i] =='^')
		release_key[i] = '^' ;
	else
		release_key[i] = map[53][(Releases[i] - 48)];
    }

  for (i = 1; i < 36 ; i += 6 ) /*up to Numr*/
    {
	if(Releases[i] =='^')
		release_key[i] = '^' ;
	else
		release_key[i] = map[54][(Releases[i] - 48)];
    }

  for (i = 2; i < 36 ; i += 6 )
    {
	if(Releases[i] =='^')
		release_key[i] = '^' ;
	else
		release_key[i] = map[53][(Releases[i] - 48)];
    }

  for (i = 3, k = 0 ; i < 36 ; i += 6, k += 5 )
    {
	if(Releases[i] =='^')
		release_key[i] = '^' ;
	else
	{
		release_key[i] = map[k + 25][(Releases[i] - 48)];
	}
    }

   for (i = 4, k = 0 ; i < 36 ; i += 6, k += 5 )
    {
        if(Releases[i] =='^')
                release_key[i] = '^' ;
        else
        {
                release_key[i] = map[k + 26][(Releases[i] - 48)];
        }
    }

   for (i = 5, k = 0 ; i < 36 ; i += 6, k += 5 )
    {
        if(Releases[i] =='^')
                release_key[i] = '^' ;
        else
        {
                release_key[i] = map[k + 27][(Releases[i] - 48)];
        }
    }



  /*------------------------------------------------------------------------*/
  /* insert keys into data to be written.                                   */
  /*------------------------------------------------------------------------*/

  for (i = 0; i < 4; i++)
    key_data[ver_offset + i] = vers_key[i];
  for (i = 0; i < 6; i++)
    key_data[prev_offset + i] = cur_date_key[i];
  for (i = 0; i < 3; i++) {
    key_data[exp_offset + i] = exp_date_key[i];
  }
  for (i = 0; i < eMAILlEN; i++)
    key_data[email_offset + i] = email_key[i];
  for (i = 0; i < 36; i++)
    key_data[release_offset + i] = release_key[i];
  for (i = 0; i < 3; i++)
    key_data[req_offset + i] = req_date_key[i];

   key_data[nr_offset] = numr_key[0];
   key_data[elen_offset] = emlen_key[0];

   key_data[KEY_DATA_LENGTH] = '\0';

}


/****************************************************************************/
/*                                                                          */
/* Function: WriteKeyFile                                                   */
/*                                                                          */
/* Description: write a special file with run time and expiration time      */
/*              encoded in it.                                              */
/*                                                                          */
/* Input: none                                                              */
/*                                                                          */
/* Output: none                                                             */
/*                                                                          */
/****************************************************************************/
int  WriteKeyFile(char *filename, unsigned char *key_data)
{
  int     rc, i;                              /* save error information  */
  int hFile;

  rc = GOOD;
  /*--------------------------------------------------------------------*/
  /* open the file for write, and check for errors                      */
  /*--------------------------------------------------------------------*/

  hFile = open(filename, O_CREAT|O_WRONLY, S_IWOTH|S_IWUSR|S_IWGRP|S_IRUSR|S_IROTH|S_IRGRP); /*|O_CREAT);//|S_IWOTH|S_IWUSR|S_IWGRP);*/

  if (hFile == -1)
  {
    printf( "Create ERROR FILE %s Failed %d %d %s\n",
               filename,
               rc,errno,
               strerror(errno));
               /*FormatError(rc));*/
    rc = FAIL;
  }

  /*-------------------------------------------------------------------*/
  /* write the file, and check the results                             */
  /*-------------------------------------------------------------------*/
  i = 0;
  if (rc == GOOD)
  {
    do {
       i++;
#ifndef __HTX_LINUX__

       if ((rc = llseek(hFile, (off_t)0, SEEK_SET)) == -1) {
          printf("Unable to set SEEK\n");
          rc = FAIL;
       }
#else

       if ((rc = lseek(hFile, (off_t)0, SEEK_SET)) == -1) {
          printf("Unable to set SEEK\n");
          rc = FAIL;
       }
#endif

       if (rc == GOOD) {
          rc = write(hFile,key_data,strlen(key_data));
          if (rc != strlen(key_data))
          {
            printf( "Write ERROR FILE %s Failed %d %s\n",
                     filename,
                     rc,
                     strerror(errno));
            rc = FAIL;
          }
          else {
            rc = GOOD;
            break;
          }
       }
    }while (i<5);
  }
  /*-------------------------------------------------------------------*/
  /* sync the file, and check the results                              */
  /*-------------------------------------------------------------------*/
  if (rc == GOOD)
  {
    rc = fsync(hFile);
    if (rc != GOOD)
    {
      printf( "Write ERROR FILE %s Failed %d errno = %d(%s)\n",
               filename,
               rc,
               errno, strerror(errno));
      rc = FAIL;
    }
  }
  /*---------------------------------------------------------------------*/
  /* Close the file at the end of the writes to it.                      */
  /*---------------------------------------------------------------------*/
  if (hFile != -1)
     close(hFile);
  return (rc);
}

/****************************************************************************/
/*                                                                          */
/* Function: ReadKeyFile                                                    */
/*                                                                          */
/* Description: read a special file with run time and expiration time       */
/*              encoded in it.                                              */
/*                                                                          */
/* Input: none                                                              */
/*                                                                          */
/* Output: none                                                             */
/*                                                                          */
/****************************************************************************/
int  ReadKeyFile(char *filename, unsigned char *key_data)
{
  int    hFile;                                 /* file handle for write   */
  int     rc, i;                                    /* save error information  */

  memset(key_data, 0, KEY_DATA_LENGTH);      /* clear the structure     */
  /*--------------------------------------------------------------------*/
  /* open the file for read, and check for errors                       */
  /*--------------------------------------------------------------------*/

  hFile = open(filename,O_RDONLY);
  if (hFile == -1) {
     rc = FAIL;
     return -1;
  }
  else
     rc = GOOD;

  /*-------------------------------------------------------------------*/
  /* read the file, and check the results                              */
  /*-------------------------------------------------------------------*/
  i = 0;
  if (rc == GOOD) {
    do {
      i++;
      rc = read(hFile, (unsigned char *)key_data, KEY_DATA_LENGTH);
      if (rc >= 0 && rc == KEY_DATA_LENGTH) {
         rc = GOOD;
         break;
      }
      else {
         printf("Could not read the file properly\n");
         rc = FAIL;
      }
    } while(i<5);
  }

  close(hFile);

  /*---------------------------------------------------------------------*/
  /* Close the file at the end of the reads to it.                       */
  /*---------------------------------------------------------------------*/

  key_data[KEY_DATA_LENGTH] = '\0';
  return (rc);
}

/****************************************************************************/
/*                                                                          */
/* Function: Activate                                                       */
/*                                                                          */
/* Description: This function is called to request an activation key, and   */
/*              decode the key into a version and expiration date.          */
/*                                                                          */
/* Input: none                                                              */
/*                                                                          */
/* Output: none                                                             */
/*                                                                          */
/****************************************************************************/
int Activate(char *version, int *exp_year, int *exp_month, int *exp_day,
             char *release, char *activation_key)
{
  int rc = GOOD;                           /* return value to caller.       */
  char in_key[STRING_LENGTH+10] = {'\0'};              /* user inputted activation key  */
  int i ;
  int cur_month = 0;
  int cur_year = 0;
  int cur_day = 0;
  int digit;
  char digit_char[5];
  char key_version[8] = "     ";
  char time_stamp[DATE_TIME_SIZE] = {'\0'};
  int  numreleases ;
  char key_releases[37] = {'\0'} ;
  int count = 0 ;
  char a_num[6] ;
  char *p;
  char buf1[10];

  a_num[1] = '\0';
  a_num[2] = '\0';

  /*------------------------------------------------------------------------*/
  /* no key passed as parameter....                                         */
  /*------------------------------------------------------------------------*/


  /*------------------------------------------------------------------------*/
  /* else copy in parameter to key for testing.                             */
  /*------------------------------------------------------------------------*/

//    strncpy(in_key, activation_key);
	memcpy(in_key, activation_key, ACTIVATION_KEY_LENGTH);

  /*------------------------------------------------------------------------*/
  /* Test the length of the key first.                                      */
  /*------------------------------------------------------------------------*/

  if (strlen(in_key) != ACTIVATION_KEY_LENGTH)
  {
    printf("Invalid Key entered\n");
    rc = FAIL;
  }
  /*-----------------------------------------------------------------------*/
  /* Extract version from the key and compare with the program version     */
  /*-----------------------------------------------------------------------*/
  else
  {


  /*-------------------------------------------------------------------------*/
  /* Extract the numrelease from key					     */
  /*-------------------------------------------------------------------------*/
        digit = map_char(in_key[nr_], 22);
	nUMr = numreleases = digit ;
	sprintf(digit_char, "%1d", digit);
        NumR[0] = digit_char[0] ;
	/*printf("In Activate, NumR = %c\n",NumR[0]); */

  strcpy(key_releases,Releases) ;
  key_releases[strlen(key_releases)] = '\0';

  if(rc == GOOD)
  {
      if(!( p = strstr(key_releases,"all")))
      {
	 rc = FAIL ;
	 verchk = 1 ;
      }

      if( rc == FAIL && (( p = strstr(Releases,release)) != NULL) )
	rc = GOOD ;


     /* if(rc == GOOD)
 	printf("\n Key valid \n"); */
  }

   /*printf("In Activate, verchk = %d\n", verchk);*/

   if(rc == GOOD && verchk)
   {
      digit = map_char(in_key[v_1], 17);
	vERSION = digit * 1000;
      sprintf(digit_char, "%1d", digit);
        key_version[0] = digit_char[0];


      digit = map_char(in_key[v_2], 18);
	vERSION = vERSION + (digit * 100);
      sprintf(digit_char, "%1d", digit);
      key_version[1] = digit_char[0];


    key_version[2] = '.';

      digit = map_char(in_key[v_3], 20);
	vERSION = vERSION + (digit * 10);
      sprintf(digit_char, "%1d", digit);
      key_version[3] = digit_char[0];

    if (in_key[4] == '_')
      key_version[4] = ' ';
    else
    {
        digit = map_char(in_key[v_4], 21);
	vERSION = vERSION + digit ;
        sprintf(digit_char, "%1d", digit);
        key_version[4] = digit_char[0];
    }
   sprintf(Version,"%s",key_version);

    if (strncmp(version, key_version,5)) {

        for(i = 0 ; i < 2 ; i++) {
                buf1[i] = version[i];
        }

        buf1[2] = '\0' ;
        Vers = strtol(buf1,NULL,10) * 100;

        for(i = 3 ; i < 5 ; i++) {
                buf1[i - 3] = version[i];
        }
        buf1[2] = '\0' ;

        Vers += strtol(buf1,NULL,10) ;

	 if (( Vers - vERSION) > 25 )
	      rc = FAIL;                             				 /* key is not for this version  */
     }
   }

    /*-----------------------------------------------------------------------*/
    /* If activation didn't fail above, extract the request date	     */
    /*-----------------------------------------------------------------------*/
    if(rc == GOOD)
    {
        req_year = map_char(in_key[y_1], 0);			/* Get the year value */

        req_month = map_char(in_key[m_1], 1);			/* Get the month      */
        (req_month)++;

        req_day = map_char(in_key[d_1], 2); 			/* Get the day        */
        (req_day)++;

	/*printf("In Activate, req_year = %d, req_month = %d, req_day = %d\n", req_year, (req_month)++, (req_day)++ );*/
    }


    /*-----------------------------------------------------------------------*/
    /* If activation didn't fail above, extract the expiration date          */
    /*-----------------------------------------------------------------------*/
    if (rc == GOOD)                         /* activation hasn't failed yet. */
    {
        *exp_year = map_char(in_key[y_2], 3);			/* Get the year value */
	/*printf("In Activate, key_year = %c, exp_year = %d\n", in_key[y_2], *(exp_year));*/

        *exp_month = map_char(in_key[m_2], 4);			/* Get the month      */
        (*exp_month)++;
	/*printf("In Activate, key_moneth = %c, exp_month = %d\n", in_key[m_2], *(exp_month));*/

        *exp_day = map_char(in_key[d_2], 5);			/* Get the day        */
        (*exp_day)++;
	/*printf("In Activate, key_day = %c, exp_day = %d\n", in_key[d_2], *(exp_day));*/

      /*---------------------------------------------------------------------*/
      /* Get the current date, and parse into numeric variables              */
      /*---------------------------------------------------------------------*/
      TimeStamp(time_stamp,DATE_TIME_SIZE);
      cur_month = strtol(time_stamp,   NULL, 10);
      cur_day   = strtol(time_stamp+3, NULL, 10);
      cur_year  = strtol(time_stamp+6, NULL, 10);
      /*printf("In Activate, cur_year = %d, cur_month = %d, cur_day = %d\n", cur_year, cur_month, cur_day);*/
      /*cur_year -= 2000;*/

      /*---------------------------------------------------------------------*/
      /* compare the key date with the current date to determine expiration  */
      /*---------------------------------------------------------------------*/
      if ((*exp_year  >= 62)         ||         /* check for invalid dates.    */
          (*exp_month >= 13)         ||
          (*exp_day   >= 32)         ||
          (cur_year  >  *exp_year)   ||
          ((cur_year == *exp_year)   && (cur_month > *exp_month)) ||
          ((cur_year == *exp_year)   && (cur_month == *exp_month) && (cur_day   >= *exp_day))) {

        rc = FAIL;
	/*printf("In Activate, Date comparison failed\n");*/
     }
    }
  }
  if (rc == FAIL) {
    printf("Invalid activation key entered\n");
    printf("Please check if the date on the machine is set to the current date OR \n");
  }
  return (rc);
}

/****************************************************************************/
/*                                                                          */
/* Function: InitialActivate                                                */
/*                                                                          */
/* Description: This function is called to try the initial key value. and   */
/*              decode the key into a version and expiration date.          */
/*                                                                          */
/* Input: none                                                              */
/*                                                                          */
/* Output: none                                                             */
/*                                                                          */
/****************************************************************************/
int InitialActivate(char *version, int *exp_year, int *exp_month, int *exp_day, char *release)
{
  int rc = GOOD;                           /* return value to caller.       */
  char in_key[STRING_LENGTH];              /* user inputted activation key  */
  char *a_char;
  int cur_month = 0;
  int cur_year = 0;
  int cur_day = 0;
  int digit;
  char digit_char[3];
  char key_version[8] = "     ";
  char time_stamp[DATE_TIME_SIZE];

  strcpy(in_key, STXAPP_KEY);
  if (strlen(in_key) != 7)
  {
    rc = FAIL;
  }

  /*-----------------------------------------------------------------------*/
  /* Extract version from the key and compare with the program version     */
  /*-----------------------------------------------------------------------*/
  else
  {
      digit = map_char(in_key[v_1], 17);
      sprintf(digit_char, "%1d", digit);
      vERSION = digit * 1000;
      if (digit != 0)
        key_version[0] = digit_char[0];


      digit = map_char(in_key[v_2], 18);
      vERSION = vERSION + (digit * 100);
      sprintf(digit_char, "%1d", digit);
      key_version[1] = digit_char[0];


    key_version[2] = '.';

      digit = map_char(in_key[v_3], 20);
      vERSION = vERSION + (digit * 10);
      sprintf(digit_char, "%1d", digit);
      key_version[3] = digit_char[0];

    if (in_key[4] == '_')
      key_version[4] = ' ';
    else
    {
        digit = map_char(in_key[v_4], 21);
	vERSION = vERSION + digit ;
        sprintf(digit_char, "%1d", digit);
        key_version[4] = digit_char[0];
      }

    if (strcmp(version, key_version))        /* compare version strings      */
      rc = FAIL;                             /* key is not for this version  */

    /*-----------------------------------------------------------------------*/
    /* If activation didn't fail above, extract the expiration date          */
    /*-----------------------------------------------------------------------*/
    if (rc == GOOD)                         /* activation hasn't failed yet. */
    {
        *exp_year = map_char(in_key[y_1], 3); 			/* Get the year value */

        *exp_month = map_char(in_key[m_1], 4);			/* Get the month      */
        (*exp_month)++;

        *exp_day = map_char(in_key[d_1], 5);			/* Get the day        */
        (*exp_day)++;

      /*---------------------------------------------------------------------*/
      /* Get the current date, and parse into numeric variables              */
      /*---------------------------------------------------------------------*/
      TimeStamp(time_stamp,DATE_TIME_SIZE);
      cur_month = strtol(time_stamp,   NULL, 10);
      cur_day   = strtol(time_stamp+3, NULL, 10);
      cur_year  = strtol(time_stamp+6, NULL, 10);
      cur_year -= 2000;

      /*---------------------------------------------------------------------*/
      /* compare the key date with the current date to determine expiration  */
      /*---------------------------------------------------------------------*/
      if (*exp_year  >= 62         ||         /* check for invalid dates.    */
          *exp_month >= 13         ||
          *exp_day   >= 32         ||
          cur_year  >  *exp_year   ||
          (cur_year == *exp_year   && cur_month > *exp_month) ||
          (cur_year == *exp_year   &&
           cur_month == *exp_month &&
           cur_day   >= *exp_day))
        rc = FAIL;
    }
  }
  return (rc);
}


void  TimeStamp(char *time_stamp, int size) {

struct tm *p_tm;
int yr2000;
long clk;

  clk = time((long *)0);

  p_tm = htx_localtime(&clk);

   if (p_tm->tm_year > 99) {
      yr2000 = p_tm->tm_year - 100;
   }
   else {
      yr2000 = p_tm->tm_year;
   }


  sprintf(time_stamp,"%.2d %.2d% .4d% .2d% .2d% .2d",(p_tm->tm_mon + 1), p_tm->tm_mday, yr2000,  p_tm->tm_hour, p_tm->tm_min, p_tm->tm_sec);
}

unsigned int Random(int seed, int min, int  max) {
int ran;
int r = 0;
int z = 0;

r = rand();
z = r % (max - min);
return(min + z);

}

int DecodeKey(unsigned char* key)
{
 int i, j, k;
  char *a_char;
  char a_num[3];
  int digit;
  char digit_char[3];
  char exp_date_key[7];
  char prev_date_key[6];
  char req_date_key[7];
  char email_key[12];
  char vers_key[4];
  char numr_key[2];
  char release_key[24];
  char temp[12];
  char emlen_key[2];
  int e[12];
  int count = 0 ;
  int vers1, vers2;
  char v[3];

  v[0] = v[1] = v[2] = '\0' ;
  a_num[1] = 0;
  a_num[2] = 0;

  e[0] = e1; e[1] = e2; e[2] = e3; e[3] = e4; e[4] = e5; e[5] = e6; e[6] = e7; e[7] = e8; e[8] = e9; e[9] = e10; e[10] = e11;

        digit = map_char(key[ne_],0);
        eMAILlEN = digit ;
        sprintf(digit_char, "%1d", digit);
        strcpy(EmailLen,digit_char);

     EmailLen[1] = '\0';

        digit = map_char(key[nr_],22);
        nUMr = digit ;
        sprintf(digit_char, "%1d", digit);
        strcpy(NumR,digit_char);

     NumR[1] = '\0';

     /*-----------------------------------------------------------------------*/
     /* decode the e-mail id                                                  */
     /*-----------------------------------------------------------------------*/

   for ( i = 0; i < eMAILlEN; i++ )
   {
        if (key[e[i]] == '@') {
                Emailid[i] = '@';
          }
        else {
        j = map_char(key[e[i]],i + 6) + 1 ;
        Emailid[i] = num2alpha(j) ;
        }
   }
        Emailid[i] = '\0';

	/*printf("In DecodeKey, Emailid = %s\n", Emailid);*/

   /*-----------------------------------------------------------------------*/
    /* decode the releases                                                   */
    /*-----------------------------------------------------------------------*/

      for(i = 0; i < 36; i++)
        Releases[i] = '^';
      Releases[36] = '\0' ;

      if(nUMr >= 1) {
        i = map_char(key[r11],53) ;
        Releases[count++] = (char)(i + 48);

        i = map_char(key[r12],54) ;
        Releases[count++] = (char)(i + 48);

        i = map_char(key[r13],53) ;
        Releases[count++] = (char)(i + 48);

	if(key[r14] != '^') /*Linux Releases*/
                {
                        i = map_char(key[r14], 25) ;
                        Releases[count++] = *((char *)(itoa( i, &a_num[0]))) ;

                        i = map_char(key[r15],26) ;
                        Releases[count++] = *((char *)(itoa( i, &a_num[0]))) ;

                        i = map_char(key[r16],27) ;
                        Releases[count++] = *((char *)(itoa( i, &a_num[0]))) ;
                }
	else
		count += 3;
     }


      if(nUMr >= 2) {
        i = map_char(key[r21],53) ;
        Releases[count++] = (char)(i + 48);

        i = map_char(key[r22],54) ;
        Releases[count++] = (char)(i + 48);

        i = map_char(key[r23],53) ;
        Releases[count++] = (char)(i + 48);

	if(key[r24] != '^') /*Linux Releases*/
                {
                        i = map_char(key[r24],30) ;
                        Releases[count++] = *((char *)(itoa( i, &a_num[0]))) ;

                        i = map_char(key[r25],31) ;
                        Releases[count++] = *((char *)(itoa( i, &a_num[0]))) ;

                        i = map_char(key[r26],32);
                        Releases[count++] = *((char *)(itoa( i, &a_num[0]))) ;
                }
	else
		count += 3;

    }


     if(nUMr >= 3) {
        i = map_char(key[r31],53);
        Releases[count++] = (char)(i + 48);

        i = map_char(key[r32],54);
        Releases[count++] = (char)(i + 48);

        i = map_char(key[r33],53) ;
        Releases[count++] = (char)(i + 48);

	if(key[r34] != '^') /*Linux Releases*/
                {
                        i = map_char(key[r34],35);
                        Releases[count++] = *((char *)(itoa( i, &a_num[0]))) ;

                        i = map_char(key[r35],36);
                        Releases[count++] = *((char *)(itoa( i, &a_num[0]))) ;

                        i = map_char(key[r36],37);
                        Releases[count++] = *((char *)(itoa( i, &a_num[0]))) ;
                }
	else
		count += 3;

     }


     if(nUMr >= 4) {
        i = map_char(key[r41],53);
        Releases[count++] = (char)(i + 48);

        i = map_char(key[r42],54);
        Releases[count++] = (char)(i + 48);

        i = map_char(key[r43],53);
        Releases[count++] = (char)(i + 48);

	if(key[r44] != '^') /*Linux Releases*/
                {
                        i = map_char(key[r44],40);
                        Releases[count++] = *((char *)(itoa( i, &a_num[0]))) ;

                        i = map_char(key[r45],41);
                        Releases[count++] = *((char *)(itoa( i, &a_num[0]))) ;

                        i = map_char(key[r46],42);
                        Releases[count++] = *((char *)(itoa( i, &a_num[0]))) ;
                }
	else
		count += 3;

     }


     if(nUMr >= 5) {
        i = map_char(key[r51],53);
        Releases[count++] = (char)(i + 48);

        i = map_char(key[r52],54);
        Releases[count++] = (char)(i + 48);

        i = map_char(key[r53],53);
        Releases[count++] = (char)(i + 48);

	if(key[r54] != '^') /*Linux Releases*/
                {
                        i = map_char(key[r54],45);
                        Releases[count++] = *((char *)(itoa( i, &a_num[0]))) ;

                        i = map_char(key[r55],46);
                        Releases[count++] = *((char *)(itoa( i, &a_num[0]))) ;

                        i = map_char(key[r56],47);
                        Releases[count++] = *((char *)(itoa( i, &a_num[0]))) ;
                }
	else
		count += 3;

     }


      if(nUMr >= 6) {
        i = map_char(key[r61],53);
        Releases[count++] = (char)(i + 48);

        i = map_char(key[r62],54);
        Releases[count++] = (char)(i + 48);

        i = map_char(key[63],53);
        Releases[count++] = (char)(i + 48);

	if(key[r64] != '^') /*Linux Releases*/
                {
                        i = map_char(key[r64],50);
                        Releases[count++] = *((char *)(itoa( i, &a_num[0]))) ;

                        i = map_char(key[r65],51);
                        Releases[count++] = *((char *)(itoa( i, &a_num[0]))) ;

                        i = map_char(key[r66],52);
                        Releases[count++] = *((char *)(itoa( i, &a_num[0]))) ;
                }
	else
		count += 3;

      }
	return 0;
	/*printf("In DecodeKey, Releases = %s\n",Releases);*/
}



int htx_license_key_validate(char *error_string)
{
  char month[3], day[3], year[5];
  int file_rc = FAIL;
  int rc = FAIL;
  int exp_year  = -1;
  int exp_month = -1;
  int exp_day   = -1;
  char activation_key[60];
  char buf[512];
  FILE *fp;
  char a_char[2];
  char *filename; /* ="/usr/lpp/htx/etc/scripts/htxkey";*/
  char buffer[35];
  char temp[15],temp1[35];
  char version[5];
  char host_name[2];
  int i,j;
  char release[7];
  char *test,*p;
  char key_filename[100];
  int fd;
  ssize_t read_rc;
  struct stat stat_buf;

  srand(time(0));
  fflush(stdout);

  host_name[2] = '\0';
  version[5] = '\0';


  filename = (char *)htx_malloc(sizeof(char) * 150);
  if(filename == NULL)
  {
	sprintf(error_string, "Error: Unable to malloc for license key validation");
	return -1;
  }


  if(!(getenv("HTXSCRIPTS")))
  {
  	strcpy(filename,"/usr/lpp/htx/etc/scripts/htxkey");
  }

  else
  {
 	strcpy(filename,getenv("HTXSCRIPTS"));
	strcat(filename,"/htxkey");
  }

  a_char[1] = '\0';

  fflush(stdout);

#ifndef __HTX_LINUX__
 fp = fopen("/usr/lpp/htx/htxaixlevel","r");

 if(fp == NULL) {
	sprintf(error_string, "Error: Unable to open /usr/lpp/htx/htxaixlevel, errno = %d\n",errno);
	return -2;
  }

        fscanf(fp,"%s",buffer);
        fscanf(fp,"%s",buffer);
        for(i=7;buffer[i]!=')';i++)
                temp[i - 7] = buffer[i];
        temp[i - 7] = '\0';
	fflush(stdout);

        if(strlen(temp) == 3)
        {
                version[0]='0';
                version[1] = temp[0];
                version[2] = '.';
                version[3] = temp[1];
                version[4] = temp[2]; version[5] = '\0';
        }
        else
        {
                version[0] = temp[0];
                version[1] = temp[1];
                version[2] = '.';
                version[3] = temp[2]; version[4] = temp[3]; version[5] = '\0';
        }

        fclose(fp);

   fp = popen("hostname","r");
   if(fp == NULL) {
	sprintf(error_string, "Error in popen while executing hostname, errno = %d\n",errno);
	return -3;
   }

        fgets(buffer,35,fp);
	host_name[0] = buffer[0] ;
	host_name[1] = '\0';
        pclose(fp);

     fp = popen(" what /usr/lpp/htx/bin/hxssup | awk '{print $5}' | grep htx | sort -u | head -n1","r");
      if(fp == NULL) {
        sprintf(error_string, "Error: in popen while executing what /usr/lpp/htx/bin/hxssup..., errno = %d\n",errno);
	return -4;
      }

    fgets(buffer,7,fp);
        pclose(fp);

        strcpy(temp,buffer);

        p = strstr(buffer,"htx5");
        release[0] = p[3]; release[1] = p[4]; release[2] = p[5]; release[3] = '\0';
  	fflush(stdout);

#else

	fp = popen("[ -f /usr/lpp/htx/etc/version ] && cat /usr/lpp/htx/etc/version || rpm -qa | grep htx","r");
	if(fp == NULL)
	{
		sprintf(error_string, "Error: Unable to get HTX Release name\n");
		return -5;
	}

	fgets(buffer,16,fp);
	pclose(fp);

	release[0] = buffer[3]; release[1] = buffer[4]; release[2] = buffer[5]; release[3] = buffer[6]; release[4] = buffer[7];
	release[5] = buffer[8]; release[6] = '\0';

	version[0] = '0'; version[1] = buffer[10]; version[2] = '.'; version[3] = buffer[11]; version[4] = buffer[12];
	version[5] = '\0';

  	fflush(stdout);

#endif


  fflush(stdout);

  /*------------------------------------------------------------------------*/
  /* Test Key file contents                                                 */
  /*------------------------------------------------------------------------*/
  rc = TestKeyFile(filename, version, &exp_year, &exp_month, &exp_day,
                                                       release, *host_name);

  /*------------------------------------------------------------------------*/
  /* If key file exists, but the version is wrong, try initial activation   */
  /*------------------------------------------------------------------------*/
  /*srand(0);*/
  switch(rc)
  {
    case OLD_VERSION:             /* an old key file from a prior version   */
      rc = InitialActivate(version, &exp_year, &exp_month, &exp_day, release);
      break;
    case NO_KEY_FILE:
      /*rc = FAIL;*/
      keyfilefound = 0 ;
      break;
//    default:
//      if (activation_key[0] != '\0')
//       printf("Activation key ignored - not needed. \n");
        /*printf("Activation key %s ignored - not needed. \n", activation_key);*/
//      break;
  }


  /*------------------------------------------------------------------------*/
  /* If key file validation failed, request an activation key.              */
  /*------------------------------------------------------------------------*/
  if (rc == NO_KEY_FILE || rc == EXPIRED || rc ==FAIL) {
                         /* key file not there, or invalid */

	if(rc == EXPIRED)
		strcpy(error_string, "Error: The key file has expired \n");


	strcpy(key_filename, " ");
	if (rc == NO_KEY_FILE) {
		if ((stat("/tmp/tempkey", &stat_buf) == 0 ) && (S_ISREG(stat_buf.st_mode))) {
			strcpy(key_filename, "/tmp/tempkey");
		}
	}

	if ( strcmp (key_filename, " ") == 0) {
		strcat(error_string, "Error: Could not find the default license key /tmp/tempkey. \nPlease copy the license key as /tmp/tempkey on the test machine");
		return -6;
	}

	fd = open(key_filename,O_RDONLY);
	if(fd == -1)
	{
		sprintf(error_string, "Error: Unable to open key file <%s>. Errno = %d\n", key_filename,errno);
		return -7;
        }


	read_rc = read(fd,buf,KEYLEN);
	if(read_rc != KEYLEN)
	{
		sprintf(error_string, "Error: Did not read the key file <%s> properly\nraed_rc = %d\n", key_filename, read_rc);
		return -8;
	}

	close(fd);


          strcpy(activation_key,buf);   activation_key[KEYLEN] = '\0' ;

	DecodeKey(activation_key);

        rc = Activate(version, &exp_year, &exp_month, &exp_day, release, activation_key);
    }


  /*------------------------------------------------------------------------*/
  /* If key file good, or activation key good, update key file.             */
  /*------------------------------------------------------------------------*/
  if (rc == GOOD)
  {

    if(verchk == 0 && keyfilefound == 0)
	    {
		file_rc = UpdateKeyFile(filename, version, exp_year, exp_month, exp_day,
                                                Releases, *host_name);
	    }
    else
  	{
	    file_rc = UpdateKeyFile(filename, Version, exp_year, exp_month, exp_day,
                                                Releases, *host_name);
	}

    if (file_rc == FAIL)               /* if update failed, reflect that to */
      rc = FAIL;                       /* the caller.                       */

    *month = exp_month;                /* set in callers variables          */
    *day = exp_day;
    *year = exp_year;
  }

  htx_free(filename);


  return(rc);
}
