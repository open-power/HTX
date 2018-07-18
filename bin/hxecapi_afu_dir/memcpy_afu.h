#ifndef _MEMCPY_AFU_H_
#define _MEMCPY_AFU_H_

#include <linux/types.h>
#include <assert.h>

struct memcpy_weq {
	struct memcpy_work_element *queue;
	struct memcpy_work_element *next;
	struct memcpy_work_element *last;
	int wrap;
	int count;
};

/*******************************************************************************
MEMCPY WORK ELEMENT memcpy_work_element 

Valid	Set by software to indicate command is ready for processing.  
Must be written last after rest of command has been written.  
Cleared by software after command is complete.

Wrap	
Wrap bit toggles each time through the FIFO.  This along with valid signifies 
if the command is valid. This protects against the engine wrapping and working 
on old commands before the software can clear the valid bit for commands that 
are completed.

Cmd	Command type to execute.
0x00: Copy
0x01: Interrupt
0x02: Stop
0x02-0x3F:  Invalid (Still to be defined)

Status	Written by AFU upon completion of command.
8: Command is complete.
9: Translation Fault “Continue”, restart mmio required
10: Aerror
11: Derror
12: PSL Fault response (not expected)
13: Failure (for interrupt command means invalid source number)
14: Process terminated
15: reserved Length

Irq Src #	For Copy Cmd: Number of bytes to copy.  Max of 2kB is all that is 
supported in current version.  Length > 2kB is invalid and will result in 
unknown behavior. Length has to be multiple of 64B in current version.  
Other lengths may be supported in future versions.

For Interrupt Cmd: Indicates source number of interrupt to be generated. 
Max src # is 2047.

Src EA	Source address of data to be copied. Only valid for Copy command type.  
Source EA must be aligned on half-line boundary. (bits 58:63 = 0).

Dest EA	Destination address for where to copy data to. Only valid for Copy 
command type.  Destination EA must be aligned on a 64B boundary (bits 58:63 = 0).
******************************************************************************/

struct memcpy_work_element {
	volatile __u8 cmd; /* valid, wrap, cmd */
	volatile __u8 status;
	__be16 length; /* also irq src */
	__be32 reserved1;
	__be64 reserved2;
	__be64 src;
	__be64 dst;
};

#define MEMCPY_WE_CMD(valid, cmd) \
	((((valid) & 0x1) << 7) |	\
	 (((cmd) & 0x3f) << 0))
#define MEMCPY_WE_CMD_COPY	0
#define MEMCPY_WE_CMD_IRQ	1
#define MEMCPY_WE_STAT_COMPLETE		0x80
#define MEMCPY_WE_STAT_TRANS_FAULT	0x40
#define MEMCPY_WE_STAT_AERROR		0x20
#define MEMCPY_WE_STAT_DERROR		0x10

#define MEMCPY_WE_CMD_VALID (0x1 << 7)
#define MEMCPY_WE_CMD_WRAP (0x1 << 6)

#define MEMCPY_WED(queue, depth)  \
	((((uintptr_t)queue) & 0xfffffffffffff000ULL) | \
	 (((__u64)depth) & 0xfffULL))

/* AFU PSA registers */
#define MEMCPY_AFU_PSA_REGS_SIZE 16*1024
#define MEMCPY_AFU_PSA_REG_CTL		0
#define MEMCPY_AFU_PSA_REG_ERR		2
#define MEMCPY_AFU_PSA_REG_ERR_INFO	3
#define MEMCPY_AFU_PSA_REG_TRACE_CTL	4

/* Per-process application registers */
#define MEMCPY_PS_REG_WED	(0 << 3)
#define MEMCPY_PS_REG_PH	(1 << 3)
#define MEMCPY_PS_REG_STATUS	(2 << 3)
#define MEMCPY_PS_REG_PCTRL	(3 << 3)
#define MEMCPY_PS_REG_TB	(8 << 3)

#define mb()   __asm__ __volatile__ ("sync" : : : "memory")

static inline int memcpy_queue_length(size_t queue_size)
{
	return queue_size/sizeof(struct memcpy_work_element);
}

void memcpy_init_weq(struct memcpy_weq *weq, size_t queue_size);
struct memcpy_work_element *memcpy_add_we(struct memcpy_weq *weq, struct memcpy_work_element we);

#endif /* _MEMCPY_AFU_H_ */
