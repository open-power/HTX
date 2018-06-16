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

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <stdint.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <fcntl.h>

#include "hxihtx64.h"
#include "zlib.h"

typedef     unsigned char       u8;
typedef     unsigned short      u18;
typedef     unsigned int        u32;
typedef     unsigned long long  u64;

typedef     signed char         s8;
typedef     signed short        s18;
typedef     signed int          s32;
typedef     signed long long    s64;


#define MAX_DEV_NAME    32
#define INPUT           0
#define OUTPUT_PASS1    1
#define OUTPUT_PASS2    2
#define MAX_BUFF        4
#define MAX_RETRIES     10

/*
 * Openx modes
 */

#define ACCEL_EXCLUSIVE_OPEN  0x1     /* Exclusive Open                */

/*
 * Adddress Translation Specification (ATS)
 */
/*
 * Macro to set a type field
 * a = address of the ATS  (or start of Child Block)
 * n = the dw offset of the region (ASIV or Child Block Descriptor words)
 * t = the type to set
 * note, if N>11, assumes "a" points to a Child Block
 */
#define ATS_FIELDS_DW0          12      /* how many fields covered by ATS dw0 */
#define ATS_FIELDS_DWS          16      /* how many fields covered by rest of */
                                        /* ATS dws                            */
#define ATS_FIELD_MASK          0xF     /* mask for 4-bit type field          */
#define ATS_DW_BITS             64      /* bits in doubleword                 */
#define ATS_BITS_PER_FIELD      4       /* bits in type field                 */
#define ATS_SHIFT_OFFSET        20      /* bit shift - type field 0 at bit    */
                                        /* position 16-20                     */

#define ATS_SET_TYPE(a, n, t)  \
        if ((n) < ATS_FIELDS_DW0)               \
                *(a) |= ((((uint64_t)(t)&ATS_FIELD_MASK)) << (ATS_DW_BITS-(((n)*ATS_BITS_PER_FIELD)+ATS_SHIFT_OFFSET))); \
        else            \
                *(a+((n+ATS_BITS_PER_FIELD)/ATS_FIELDS_DWS)) |= ((((uint64_t)(t)&ATS_FIELD_MASK)) << ((ATS_DW_BITS-(((n-ATS_FIELDS_DW0-1)%ATS_FIELDS_DWS*ATS_BITS_PER_FIELD)))%ATS_DW_BITS));

/*
 * Macro to extract a type field
 * a = address of the ATS  (or start of Child Block)
 * n = the dw offset of the region (ASIV or Child Block Descriptor words)
 * note, if N>11, assumes "a" points to a Child Block
 */
#define ATS_GET_TYPE(a, n)  \
        (((n) < ATS_FIELDS_DW0) ? (*(a) >> (ATS_DW_BITS-(((n)*ATS_BITS_PER_FIELD)+ATS_SHIFT_OFFSET)) & ATS_FIELD_MASK) : (*(a+((n+ATS_BITS_PER_FIELD)/ATS_FIELDS_DWS)) >> ((ATS_DW_BITS-(((n-ATS_FIELDS_DW0-1)%ATS_FIELDS_DWS*ATS_BITS_PER_FIELD)))%ATS_DW_BITS) & ATS_FIELD_MASK))

/*
 * ATS Field Type Definitions (4-bit fields)
 */
#define ATS_RAW         0x0     /* Raw Data, not an address          */
#define ATS_CHILDB      0x2     /* Child Block address (512B)        */
#define ATS_FLAT_R      0x4     /* 8B read-only addr,4B len,4B data  */
#define ATS_FLAT_RW     0x5     /* 8B read-write addr,4B len,4B data */
#define ATS_SG_R        0x6     /* 8B read-only SGptr,4B len,4B data */
#define ATS_SG_RW       0x7     /* 8B read-write SGptr,4B len,4B data*/



#define ACCEL_NUM_ASV_DWORDS 12 /* Number of double words in ASVs      */

#define ACCEL_NUM_ASV_WORDS  24 /* Number of double words in ASVs      */

/*
 * Child Block
 */


#define ACCEL_NUM_CHILD_ATS 4 /* Number of ATS in child block     */

#define ACCEL_NUM_CHILD_DESC_DWORDS 60  /* Total number of double */
					/* words descriptors in   */
					/* child blocks           */

typedef struct _child_blk {

        uint64_t ats[ACCEL_NUM_CHILD_ATS];

        uint64_t desc0[12];
        uint64_t desc1[16];
        uint64_t desc2[16];
        uint64_t desc3[16];
} child_blk_t;

/*
 * Flat Address Descriptor
 */
typedef struct _flat_addr {
        uint64_t addr;
        uint32_t len;
        uint32_t data;
} flat_addr_t; 

/*
 * Scatter Gather List Descriptor
 */
typedef struct _sg_list_desc {
        uint64_t addr;
        uint32_t len;
        uint32_t data;
} sg_list_desc_t;

/*
 * Scatter Gather List Entry
 */
typedef struct _sg_list_entry {
        uint64_t addr;
        uint32_t len;
        unsigned rsvd:28;
#define SG_FLAGS_END_LIST       0x0
#define SG_FLAGS_DATA           0x2
#define SG_FLAGS_CHAIN          0x6
        unsigned flags:4;
} sg_list_entry_t;

/*****************************************************************************/
/*  Version numbers of structures in this file.                              */
/*****************************************************************************/

#define ACCEL_DEV_VER_0   0x00     /* First version of the structure. */



/* Below structures are used for issuing IOCTL in AIX.
 * Few #defines collide with Linux header for corsa.
 * Enabling them only for AIX 
 */
#ifndef __HTX_LINUX__
/*
 * IOCTLs
 */
#define ACCEL_ENQ_CMD           1  /* Enqueue DDCB Command        */
#define ACCEL_ABORT_CMD         2  /* Abort DDCB Command          */
#define ACCEL_LOAD_IMAGE        3  /* Load accelerator image      */
#define ACCEL_MMIO              4  /* Issue MMIO request          */
#define ACCEL_QUERY_IMAGE       5  /* Query accelerator image     */

/*
 * Application Specific Info
 */
typedef struct _app_info {
                                                             
        uint64_t ats;           /* Address translation fields          */

        union _asiv{            /* Application Specific Invariant (96 bytes) */
                uint64_t    as_uint64s[ACCEL_NUM_ASV_DWORDS];
                uint32_t    as_uint32s[ACCEL_NUM_ASV_WORDS];
        } asiv;
  
        uint64_t    asv[8];     /* Application Specific Variant (64 bytes) */
        uint16_t    rsvd2;      /* Reserved                            */
        uint16_t    vcrc;       /* Variant CRC                         */
        uint32_t    rsvd3;      /* Reserved                            */

        uint64_t    dequeue_ts; /* Dequeue Time Stamp                  */

        uint16_t    retc;       /* Return Code - Bitwise Encoding      */
#define DDCB_RETC_NOT_COMPLETE  0      /* unexecuted, purged, or DDCB created */
#define DDCB_RETC_PARITY        0x100  /* bit set if valid completion  */
#define DDCB_RETC_PENDING       0x1    /* pending execution, fetched   */
#define DDCB_RETC_COMPLETE      0x2    /* command complete, no error   */
#define DDCB_RETC_ERR_RECOV     0x4    /* error,  but recoverable      */
#define DDCB_RETC_ERR_NONRECOV  0x8    /* error, but non-recoverable   */
#define DDCB_RETC_SL_UNEX_RECOV 0x10   /* service layer removed, not processed*/
#define DDCB_RETC_SL_TERM_NR    0x20   /* service layer terminated, not recov*/
#define DDCB_RETC_SL_TERM_TO    0x40   /* service layer terminated, timeout */

        uint16_t    attn;       /* Attention / Extended Error Codes     */

        uint32_t    progress;   /* Progress Indicator                   */

        uint64_t    comp_ts;    /* Completion Time Stamp                */

        uint32_t    ibdc;       /* Inbound Data Count                   */
        uint32_t    obdc;       /* Outbound Data Count                  */
        uint64_t    rsvd4;      /* Reserved                             */
        uint64_t    fw;         /* Firmware                             */
        uint64_t    disp_ts;    /* Dispatch Time Stamp                  */
} app_info_t;

/*
 * ioctl struct for ACCEL_ENQ_CMD
 */
typedef struct _accel_enq_cmd {
        uint8_t version;        /* The version number of this     */
                                /* structure.  This allows the    */
                                /* structure to expand in the     */
                                /* future.                        */
        uint8_t cmd;            /* Accelerator Command            */
        uint8_t rsvd1;          /* Reserved for future use        */
        uint16_t cmdopts;       /* Command Options                */
        uint16_t ac_func;       /* Accelerator Function           */
				/* 0=Service layer,  1=application*/
        uint16_t in_flags;
#define ACCEL_FLG_ASYNC     0x1 /* Non-blocking command issue,    */
				/* default is SYNC                */
#define ACCEL_FLG_MAPONLY   0x2 /* Only DMA map the buffers       */
				/* described by ATS               */
#define ACCEL_FLG_UNMAP     0x4 /* Unmap buffers described by ATS */

#define ACCEL_FLG_SIGNAL    0x8 /* If the ACCEL_FLG_ASYNC is also */
				/* set and then send a signal to  */
				/* processwhich issued this       */
				/* request                        */

        uint32_t timeout;       /* Time that the command should   */
				/* complete (seconds) 0 = no      */
				/* timeout                        */
        uint8_t rsvd2;

        app_info_t  app;        /* Application specific command   */
				/* and status                     */

        uint64_t  request_id;   /* Request ID,  filled out upon   */
				/* successful enqueue             */
        uint32_t  out_flags;
#define ACCEL_CMD_ACTIVE    0x1 /* request is active              */
#define ACCEL_CMD_ENQUEUED  0x2 /* request is enqueued to adapter */
#define ACCEL_CMD_COMPLETED 0x4 /* request completed              */
#define ACCEL_CMD_TIMEOUT   0x8 /* request timedout               */
#define ACCEL_CMD_ABORTED  0x10 /* request aborted                */
        uint32_t  rsvd3;        /* Reserved for future use        */
        uint32_t  rsvd4;        /* Reserved for future use        */

} accel_enq_cmd_t;

/*
 * ioctl struct for ACCEL_LOAD_IMAGE
 * (Requires Exclusive Open and Root Privilege)
 */
typedef struct _load_image {
        uint8_t version;        /* The version number of this  */
                                /* structure.  This allows the */
                                /* structure to expand in the  */
                                /* future.                     */
	uint8_t rsvd1;          /* Reserved for future use     */
        uint32_t status;        /* status returned to caller.  */
        uint32_t flags;         /* Flags set by caller         */
#define ACCEL_LOAD_IMAGE_BKUP_FLG   0x1  /* Update back up     */
					 /* image on devioe    */
#define ACCEL_LOAD_IMAGE_VERIFY_FLG 0x2  /* Verify image, but  */
				         /* but do not update  */
				         /* device             */
        uint32_t ret_flags;     /* Flags returned to caller    */
#define ACCEL_LOAD_IMAGE_STAT_VAL   0x1  /* status   field     */
					 /* valid              */
#define ACCEL_LOAD_IMAGE_PROG_VAL   0x2  /* Progress field     */
					 /* valid              */
        uint64_t userbuffer;    /* User buffer of image to be  */
				/* loaded                      */
        uint64_t size;          /* size in bytes of User       */
				/* buffer.                     */
        uint64_t progress;      /* progress indicator returned */
				/* to caller.                  */
} accel_load_image_t;

/*
 * ioctl struct for ACCEL_MMIO
 * (Requires Exclusive Open)
 */
typedef struct _accel_mmio{
        uint8_t version;        /* The version number of this     */
                                /* structure.  This allows the    */
                                /* structure to expand in the     */
                                /* future.                        */
        uint8_t rsvd1;          /* Reserved for future use        */
        uint64_t read_write_flag;
#define ACCEL_MMIO_R            0
#define ACCEL_MMIO_W            1
        uint64_t offset;
        uint64_t value;
        uint32_t  rsvd2;        /* Reserved for future use        */
} accel_mmio_t;


/*
 * ioctl struct for ACCEL_QUERY_IMAGE
 */
typedef struct _accel_query_image{
        uint8_t version;        /* The version number of this     */
                                /* structure.  This allows the    */
                                /* structure to expand in the     */
                                /* future.                        */
        uint8_t rsvd1;          /* Reserved for future use        */
        uint32_t flags;         /* Flags set by caller            */
        uint32_t ret_flags;     /* Flags returned to caller       */
#define ACCEL_QRY_BKUP_SPT  0x1 /* Device supports back up        */
				/* load image                     */ 
#define ACCEL_QRY_VERIFY_SPT 0x2 /* Device supports verification  */
				/* of load image                  */  
        uint64_t reserved;      /* Reserved for future use        */
        union {
	    uint64_t pci;       /* PCI device ID                  */
	    struct {
		uint64_t id;    
		uint64_t version;
	    } dev;
        } device_id;
        uint64_t app_id;        /* Application Identifier         */
        uint32_t  rsvd2;        /* Reserved for future use        */
        uint32_t  rsvd3;        /* Reserved for future use        */
} accel_query_image_t;

/*
 * Generic Device Driver Control Block (DDCB) Structure
 */
#pragma pack(1)
typedef struct _generic_ddcb {

        uint16_t    icrc;       /* Invariant CRC */
        uint8_t     hsi;        /* Hardware to Software Interlock */
#define DDCB_HSI_COMPLETED      0x40 /* HW made last update to this DDCB */
#define DDCB_HSI_FETCHED        0x04 /* HW copied invariant, can't purge */
        uint8_t     shi;        /* Software to Hardware Interlock */
#define DDCB_SHI_INTR           0x4 /* Enable interrupt on completion    */
#define DDCB_SHI_PURGE          0x2 /* Discard current queue entry       */
#define DDCB_SHI_NEXT           0x1 /* Subsequent queue entry valid      */
        /* Begin Invariant Section (byte 0x04, up to 0x80) */
        uint8_t     pre;        /* Preamble */
#define DDCB_PRE_VALID          0x80 /* bit 7                            */
#define DDCB_PRE_ARCH_V0        0    /* First Generation                 */
#define DDCB_PRE_ARCH_MASK      0x70 /* bits 4:6                         */
#define DDCB_PRE_ARCH_SHIFT     4    /* bits 4:6                         */
#define DDCB_PRE_RSVD_MASK      0x0F /* bits 0:3                         */
#define DDBB_PRE(ver)   (DDCB_PRE_VALID | (((ver) << DDCB_PRE_ARCH_SHIFT) & DDCB_PRE_ARCH_MASK)) 

        uint8_t     xdir;       /* Execution Directives */
#define DDCB_XDIR_CONT          1  /* Continue on Recoverable errors */

        uint16_t    seqnum;     /* Sequence Number */

        uint8_t     acfunc;     /* Accelerator Function */

#define DDCB_ACFUNC_SERVQ       0  /* Service Layer Queue */
#define DDCB_ACFUNC_APPQ        1  /* Custom Application Queue */

        uint8_t     cmd;        /* Command */
#define DDCB_CMD_ECHO          0 /* Echo/Sync command. It copies data    */
			         /* from invariant section of DDCB to    */
			         /* the variant part of the output part  */
#define DDCB_CMD_MEMCPY        3 /* Copies data from specified input     */
			         /* buffer to specified output buffer    */
#define DDCB_CMD_MOVE_FLASH    6 /* Copy data between host and FPGA flash*/
			         /* memory.                              */    
  
#define DDCB_CMD_PARTIAL_RECFG 7       
        uint16_t    cmdopts;    /* Command Options */
        uint8_t     sur;        /* Status Update Rate(not currently used)*/
        union _psp {
          struct _pspbits {
                unsigned inv:4; /* Protection Section Pointer (invariant)*/
#define DDCB_PSP_INV(bytes) (((bytes) - 20) / 8)
                unsigned var:4; /* Protection Section Pointer (variant)  */
#define DDCB_PSP_VAR(bytes) (((bytes) / 8)
          } pspbits;
          uint8_t pspbyte;
         } _pspu;

        uint16_t    rsvd1;      /* Reserved                              */

        uint64_t    fwiv;       /* Firmware/software Invariant           */

        app_info_t  appinfo;    /* Application Specific ATS, ASIV, ASV   */

} generic_ddcb_t;
#pragma pack(pop)

#else /* __HTX_LINUX__ */

struct asiv_memcpy {
	uint64_t inp_buff;  /**< 0x20 input buffer address */
	uint32_t inp_buff_len;  /**< 0x28 */
	uint32_t in_crc32;  /**< 0x2c */

	uint64_t outp_buff; /**< 0x30 input buffer address */
	uint32_t outp_buff_len; /**< 0x38 */
	uint32_t in_adler32;    /**< 0x3c */
	uint64_t  res[8];   /**< 0x40 ... 0x7f */
} __attribute__((__packed__)) __attribute__((__may_alias__));

/**
 *  * application specific variant part of the DDCB (56 bytes: 0x80...0xb7)
 *   * see ZCOMP Data Compression HLD spec 0.96: 5.3.3 Mmcopy CMD
 *    */
struct asv_memcpy {
	uint64_t res0[2];   /**< 0x80 ... 0x8f */
	uint32_t out_crc32; /**< 0x90 */
	uint32_t out_adler32;   /**< 0x94 */
	uint32_t inp_processed; /**< 0x98 */
	uint32_t outp_returned; /**< 0x9c */
	uint64_t  res1[4];  /**< 0xa0 ... 0xbf */
} __attribute__((__packed__)) __attribute__((__may_alias__));
 
#endif /* __HTX_LINUX__ */


typedef struct {
        int buf_size;
        int alloc_size;
        int alignment;
        char *ea;
        char *ea_org;
		int shm_id;
        u64 ra;
        /* Making xm as an array of 4, becoz same buf will get used to find
         * real address of all virtual 4K pages in the buffer (Max 4).
         */
        int indirect_dde_buf_len;
        int org_indirect_dde_buf_len;
        void *indirect_dde_buf;
}buf_addr;


typedef struct {
        pthread_cond_t thread_cond_wt;
        int num_buf;
        int mode_of_exec;
        buf_addr all_buf[MAX_BUFF];
        buf_addr *i_buf;
        buf_addr *o_buf;

        buf_addr csbcpb_info_buf[MAX_BUFF];
        buf_addr *cur_csbcpb_info_buf;

        s64 inlen;
        s64 outlen_pass1;
        s64 outlen_pass2;

        s64 *inlen_ptr;
        s64 *outlen_ptr;
        u64 timer_ticks;
        u64 start_tb;
        u64 end_tb;


        int corsa_fd;

        FILE *dump_file;

	u32 cmd_pad;
#ifndef __HTX_LINUX__
        accel_enq_cmd_t corsa_buf;
#else
	struct genwqe_ddcb_cmd corsa_buf;
#endif
}th_context;

typedef enum {
        /* All the crypto algos to be added here. */
        SHA_1,
        SHA_256,
        SHA_512,
        MD_5,
        SHA1_HMAC,
        SHA256_HMAC,
        SHA512_HMAC,
        AES_ECB,
        AES_CBC,
        AES_CTR,
        AES_GCA,
        AES_CCA,
        AES_GMAC,
        AES_XMAC,
        AES_CCM,
        AES_GCM,
        AES_CBC_HMAC_EtA,
        AES_CBC_HMAC_AtE,
        AES_CBC_HMAC_EaA,
        AES_CTR_HMAC_EtA,
        AES_CTR_HMAC_AtE,
        AES_CTR_HMAC_EaA,
        AES_CBC_HMAC_DtA,
        AES_CBC_HMAC_AtD,
        AES_CBC_HMAC_DaA,
        AES_CTR_HMAC_DtA,
        AES_CTR_HMAC_AtD,
        AES_CTR_HMAC_DaA,
        MMult,
        MgMult,
        MExp,
        MExpCRT,
        MRed,
        NMRed,
        MInv,
        MAdd,
        MSub,
        PAdd,
        PDbl,
        PMul,
        M2Mult,
        Mg2Mult,
        M2Exp,
        M2Red,
        M2Add,
        M2Inv,
        P2Add,
        P2Dbl,
        P2Mult,
        compression_with_crc,
        compression_without_crc,
        decompression_with_crc,
        decompression_without_crc,
        data_bypass,
        corsa_deflate,
        corsa_deflate_crc,
        corsa_inflate,
        corsa_inflate_crc,
        corsa_memcopy
} ALGOS;

