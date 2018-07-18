#define _BSD_SOURCE
#define __STDC_FORMAT_MACROS
#define _XOPEN_SOURCE /* For ipc.h */
#define _ISOC11_SOURCE

#include <sys/time.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/wait.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/select.h>
#include <inttypes.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <poll.h>
#include <linux/types.h>
#include <linux/kernel.h>
#include <endian.h>
#include <time.h>
#include <getopt.h>

#include "memcpy_afu.h"

/* master mmap size (hex)
   slave mmap size (hex)
   per-process offeset (hex)
   afu configuration records (don't worry about it for the moment)
   num of processes (int)
   supported programming models (list of strings)
   current programming models (rw string)
   CAIA version (string)
   PSL version (string)
*/

/* Generic library functions for the memcpy test AFU */

void memcpy_init_weq(struct memcpy_weq *weq, size_t queue_size)
{
	weq->queue = aligned_alloc(getpagesize(), queue_size);
	memset(weq->queue, 0, queue_size);
	weq->next = weq->queue;
	weq->last = weq->queue + memcpy_queue_length(queue_size) - 1;
	weq->wrap = 0;
	weq->count = 0;
}

/*
 * Copies a work element into the queue, taking care to set the wrap bit correctly.
 * Returns a pointer to the element in the queue.
 */
struct memcpy_work_element *memcpy_add_we(struct memcpy_weq *weq, struct memcpy_work_element we)
{
	struct memcpy_work_element *new_we = weq->next;

	new_we->length = we.length;
	new_we->src = we.src;
	new_we->dst = we.dst;
	new_we->status = we.status;
	mb();
	new_we->cmd = (we.cmd & ~MEMCPY_WE_CMD_WRAP) | weq->wrap;
	weq->next++;
	if (weq->next > weq->last) {
		weq->wrap ^= MEMCPY_WE_CMD_WRAP;
		weq->next = weq->queue;
	}

	return new_we;
}
