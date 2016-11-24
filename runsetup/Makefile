include ../htx.mk

TARGET= ablink.runsetup xyzlink.runsetup cpu.runsetup fpu.runsetup \
	mem.runsetup hxecom.runsetup hxediag.runsetup

.PHONY: all clean

all: ${TARGET}
	@echo "making dir - "${RUNSETUP}
	${MKDIR} ${RUNSETUP}
	${CP} ${TARGET} ${RUNSETUP}

clean:
	${RM} -rf ${RUNSETUP}/*.runsetup
