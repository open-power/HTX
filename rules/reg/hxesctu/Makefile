include ../../../htx.mk

TARGET= \
	rules.default \
	default.p9 \
    rules.64.SAO \
    rules.CacheBias \
    rules.InterNode \
    rules.LwarxStwcx \
	rules.chip \
	rules.node \
    hxesctu.readme

.PHONY: all clean

all: ${TARGET}
	${MKDIR} ${REGRULES}/hxesctu/
	${CP} ${TARGET} ${REGRULES}/hxesctu/ 

clean:
	${RM} -rf ${REGRULES}/hxesctu/*
