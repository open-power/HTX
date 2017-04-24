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
/*@(#)49  1.5.3.1  src/htx/usr/lpp/htx/bin/hxssup/hxssup.h, htx_sup, htxubuntu 6/24/14 01:19:21*/
#ifndef HXSSUP_H
#define HXSSUP_H

/*
 ******************************************************************************
 ***  hxssup.h -- "hxssup" HTX Supervisor Include File  ***********************
 ******************************************************************************
 */


#include <curses.h>

#ifdef	__HTX_LINUX__
#include <sys/param.h>
#include <htx_local.h>
#include <hxiipc.h>
#include <hxiconv.h>
/*union semun{
	int val;
	struct semid_ds *buf;
	unsigned short *array;
	struct seminfo *__buf;
	void *__pad;
};*/

#else
#include <htx_local.h>
#include <hxiipc.h>
#include <hxiconv.h>
#endif


/* undefine TRUE and FALSE to prevent lint errors -- redefined in cfgdb.h */
#undef TRUE
#undef FALSE

#ifdef	__HTX_LINUX__
#define	TRUE	1
#define	FALSE	0
#endif

#ifndef	__HTX_LINUX__
#include <sys/cfgdb.h>
#include <sys/cfgodm.h>
#endif

#ifdef __HTX_LINUX_
    #define ENTER_KEY 10
#else
    #define ENTER_KEY 13
#endif

#define ESC_KEY 27
#include "hxssupdef.h"
#define HTXSHMKEY 10601

#define RULE_NAME_LENGTH_EXCEEDED 5001

#endif				/* HXSSUP_H */
