include ../../../htx.mk

TARGET = default.ssd \
        default.hdd \
        ssd.buster \
        hdd.buster \
        default.worm \
        default.dvd \
        hdd.RD_BW \
        hdd.WR_BW \
        hdd.RD_IOPs \
        hdd.WR_IOPs \
        ssd.RD_BW \
        ssd.WR_BW \
        ssd.RD_IOPs \
        ssd.WR_IOPs \
        easytier \
        certify \
        rules.app_server \
        rules.web_server \
        rules.business_int_db_opk \
        rules.business_int_db_pk \
        rules.comm_db_opk \
        rules.comm_db_pk \
        rules.hpc_comp_node \
        rules.hpc_io_node \
        hd.cache \
        rules.cache_stress \
        rules.discard \
        rules.sync_cache \
        storage.coral.pass1 \
        storage.coral.pass1 \
        hxestorage.readme

.PHONY: all clean

all: ${TARGET}
	${MKDIR} ${REGRULES}/hxestorage/
	${CP} ${TARGET} ${REGRULES}/hxestorage/ 

clean:
	${RM} -rf ${REGRULES}/hxestorage/* 
