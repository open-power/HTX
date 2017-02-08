include ../htx.mk

TARGET= htx_eq.cfg 

.PHONY: all clean

all:
	@echo "making dir - "${CFGPATH}
	${MKDIR} ${CFGPATH}
	${CP} ${TARGET} ${CFGPATH}

clean:
	@echo "Removing dir - "${CFGPATH}
	${RM} -rf ${CFGPATH}
