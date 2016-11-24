include ../htx.mk

TARGET= hxecom.runcleanup hxediag.runcleanup

.PHONY: all clean

all: ${TARGET}
	@echo "making dir - "${RUNCLEANUP}
	${MKDIR} ${RUNCLEANUP}
	${CP} ${TARGET} ${RUNCLEANUP}

clean:
	${RM} -rf ${RUNCLEANUP}/*.runsetup
