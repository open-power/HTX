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

#include "memcpy.h"

void memcpy3_init_weq(struct memcpy_weq *weq, size_t queue_size)
{
        weq->queue = aligned_alloc(getpagesize(), queue_size);
        memset(weq->queue, 0, queue_size);
        weq->next = weq->queue;
        weq->last = weq->queue + memcpy3_queue_length(queue_size) - 1;
        weq->wrap = 0;
        weq->count = 0;
}

/*
 * Copies a work element into the queue, taking care to set the wrap
 * bit correctly.  Returns a pointer to the element in the queue.
 */
struct memcpy_work_element *memcpy3_add_we(struct memcpy_weq *weq,
                                        struct memcpy_work_element we)
{
        struct memcpy_work_element *new_we = weq->next;

        new_we->status = we.status;
        new_we->length = we.length;
        new_we->cmd_extra = we.cmd_extra;
        new_we->atomic_op = we.atomic_op;
        new_we->src = we.src;
        new_we->dst = we.dst;
        mb();
        new_we->cmd = (we.cmd & ~MEMCPY_WE_CMD_WRAP) | weq->wrap;
        weq->next++;
        if (weq->next > weq->last) {
                weq->wrap ^= MEMCPY_WE_CMD_WRAP;
                weq->next = weq->queue;
        }

        return new_we;
}

/**
 * Restart the AFU if it is stopped
 *
 * @param pp_mmio the per-PASID MMIO area of the AFU to restart
 * @return false on success, true on failure
 */
int restart_afu_if_stopped(ocxl_mmio_h pp_mmio)
{
    /* Allow the AFU to proceed */
    if (OCXL_OK != ocxl_mmio_write64(pp_mmio, MEMCPY_AFU_PP_CTRL, OCXL_MMIO_LITTLE_ENDIAN, MEMCPY_AFU_PP_CTRL_Restart)) {
                return 1;
        }

        return 0;
}

int wait_for_status(int timeout, struct memcpy_work_element *we, ocxl_afu_h afu, uint64_t err_ea)
{
    struct timeval test_timeout, temp;

    temp.tv_sec = timeout;
    temp.tv_usec = 0;

    /* printf("timeout: %d\n", timeout); */
    gettimeofday(&test_timeout, NULL);
    timeradd(&test_timeout, &temp, &test_timeout);

    for (;; gettimeofday (&temp, NULL)) {
        if (timercmp (&temp, &test_timeout, >)) { /* timeout polling for completion */
            return -1;
        }
        int ret = wait_for_irq(0, afu, 0, 0, err_ea);
        if (ret) {
            return ret;
        }

        if (we->status) {
            break;
        }
    }
    return 0;
}

int wait_for_irq(int timeout, ocxl_afu_h afu, ocxl_mmio_h pp_mmio,
                 uint64_t irq_ea, uint64_t err_ea)
{
    ocxl_event event;
    int rc = 0, nevent;
    int check_timeout = timeout * 1000; /* convert to milliseconds */

    do {
        nevent = ocxl_afu_event_check (afu, check_timeout, &event, 1);
        if (nevent < 0) {
            return 0x04;
        }
        if (nevent == 0) {
            if (timeout) {  /* Timeout waiting for interrupt */
                return rc |= 0x08;
            }
            break;
        }
        /* No need to wait if we go around the loop again */
        check_timeout = 0;
        switch (event.type) {
            case OCXL_EVENT_IRQ:
                if (irq_ea && event.irq.handle == irq_ea) { /* We have an AFU interrupt */
                    restart_afu_if_stopped(pp_mmio);
                    return 0;
                } else if (event.irq.handle == err_ea) { // We have an AFU error interrupt
                    rc |= 0x01;
                }
                break;
            case OCXL_EVENT_TRANSLATION_FAULT:
                /* printf("Translation fault detected, addr=%p count=%lu\n",
                                        event.translation_fault.addr, event.translation_fault.count); */
                rc |= 0x02;
                break;
        }
    } while (nevent == 1); // Go back around in case there are more events to process

    return rc;
}
