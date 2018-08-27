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

#include <linux/types.h>
#include <sys/time.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <time.h>
#include <libocxl.h>
#include <stdlib.h>

#define CACHELINESIZE   128
/* Queue sizes other than 512kB don't seem to work (still true?) */
#define QUEUE_SIZE  4095*CACHELINESIZE
#define MEMCPY_SIZE     2048

#define mb()   __asm__ __volatile__ ("sync" : : : "memory")

#define MEMCPY_WED(queue, depth)            \
    ((((__u64)queue) & 0xfffffffffffff000ULL) | \
        (((__u64)depth) & 0xfffULL))

#define MEMCPY_WE_CMD(valid, cmd)       \
    (((valid) & 0x1) |          \
        (((cmd) & 0x3f) << 2))
#define MEMCPY_WE_CMD_VALID (0x1 << 0)
#define MEMCPY_WE_CMD_WRAP  (0x1 << 1)
#define MEMCPY_WE_CMD_COPY      0
#define MEMCPY_WE_CMD_IRQ       1
#define MEMCPY_WE_CMD_STOP      2
#define MEMCPY_WE_CMD_WAKE_HOST_THREAD  3
#define MEMCPY_WE_CMD_INCREMENT     4
#define MEMCPY_WE_CMD_ATOMIC        5
#define MEMCPY_WE_CMD_TRANSLATE_TOUCH   6

/* global mmio registers */
#define MEMCPY_AFU_GLOBAL_CFG   0
#define MEMCPY_AFU_GLOBAL_TRACE 0x20

/* per-process mmio registers */
#define   MEMCPY_AFU_PP_WED 0
#define   MEMCPY_AFU_PP_STATUS  0x10
#define   MEMCPY_AFU_PP_STATUS_Terminated   0x8
#define   MEMCPY_AFU_PP_STATUS_Stopped      0x10

#define   MEMCPY_AFU_PP_CTRL    0x18
#define   MEMCPY_AFU_PP_CTRL_Restart    (0x1 << 0)
#define   MEMCPY_AFU_PP_CTRL_Terminate  (0x1 << 1)
#define   MEMCPY_AFU_PP_IRQ 0x28

struct memcpy_work_element {
    volatile __u8 cmd; /* valid, wrap, cmd */
    volatile __u8 status;
    __le16 length;
    __u8 cmd_extra;
    __u8 reserved[3];
    __le64 atomic_op;
    __le64 src;  /* also irq EA or atomic_op2 */
    __le64 dst;
} __packed;

struct memcpy_weq {
    struct memcpy_work_element *queue;
    struct memcpy_work_element *next;
    struct memcpy_work_element *last;
    int wrap;
    int count;
};

static inline int memcpy3_queue_length(size_t queue_size)
{
        return queue_size/sizeof(struct memcpy_work_element);
}

void memcpy3_init_weq(struct memcpy_weq *weq, size_t queue_size);
struct memcpy_work_element *memcpy3_add_we(struct memcpy_weq *weq, struct memcpy_work_element we);
int wait_for_status (int timeout, struct memcpy_work_element *we, ocxl_afu_h afu, uint64_t err_ea);
int wait_for_irq (int timeout, ocxl_afu_h afu, ocxl_mmio_h pp_mmio,
                 uint64_t irq_ea, uint64_t err_ea);
int restart_afu_if_stopped(ocxl_mmio_h pp_mmio);
