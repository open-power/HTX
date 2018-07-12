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
/*
 * 4mm_auto.h
 *
 * Interface include file for DDS2 SCSI drive driver for IBM RS-6000
 * 
 *  Version %V%   Created on 2/27/92 at 13:11:45
 * 
 */

#ifndef  _H_DDS2
#define _H_DDS2

#include <sys/types.h>

/************************************************************************/
/* Ioctl Command defines                                                */
/************************************************************************/

#define INIT_ELEMENT		0x07
#define MOVE_MEDIUM_LOAD	0xb4
#define MOVE_MEDIUM_UNLOAD	0xb5
#define READ_ELEMENT_STATUS	0xb8

/* For locate ioctl, DDS2_LOCATE is opcode and arg is (uint) block number */
#define DDS2_LOCATE		0x2b
/* For READ POSITION ioctl, DDS2_READ_POS is opcode and arg POINTS to place to
 * store current block position
 */
#define DDS2_READ_POS		0x34

#define DDS2_CONSTRAINT_ON	1
#define DDS2_CONSTRAINT_OFF	0

#ifndef __HTX_LINUX__
/* If this is 3.1, tape.h does not define a rewind/unload command.  add it. */
#ifndef STOFFL
#define STOFFL	5		/* REWIND UNLOAD */
#endif
#endif

/************************************************************************/
/* Initialization information on individual drives                       */
/************************************************************************/

typedef struct dds2_dds {
    dev_t	adapter_devno;
    char	scsi_id;
    char	lun_id;
    char	dev_name[16];
    int		debug_level;
    int 	minor_array[16];	/* valid minor numbers */
    int		dev_blksize;
    int		buffered_write;
    int		compress;
    uint	reserved;
} DDS2_DDS;

struct element_status {
    int 	n_elements;	/* number of elements to report on */
    int		stat_len;	/* allocation length */
    char	*element_stat;	/* element status buffer */
};

#endif /* ! _H_DDS2 */
