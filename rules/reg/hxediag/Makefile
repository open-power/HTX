include ../../../htx.mk

TARGET= \
	default default.ib default.ib_gp default.mfg hxediag.readme ib_gp1p

.PHONY: all clean

all: ${TARGET}
	${MKDIR} ${REGRULES}/hxediag/
	${CP} ${TARGET} ${REGRULES}/hxediag/ 

clean:
	${RM} -rf ${REGRULES}/hxediag/* 
