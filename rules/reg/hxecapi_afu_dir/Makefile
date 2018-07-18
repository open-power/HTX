include ../../../htx.mk

TARGET= \
        default 

.PHONY: all clean

all: ${TARGET}
	${MKDIR} ${REGRULES}/hxecapi_afu_dir/
	${CP} ${TARGET} ${REGRULES}/hxecapi_afu_dir/

clean:
	${RM} -rf ${REGRULES}/hxecapi_sfu_dir/*

