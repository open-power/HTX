include ../../../htx.mk

TARGET= \
	default.p6 default.p7 default.p8 default.p9 default.eco.p8 default.eco.p9 rules.noprefetch rules.noprefetch.p9  \
	hxecache.readme default.sw_prefetch default.sw_prefetch.p8 default.sw_prefetch.p9 default.cache.eq rules.l2_bounce.p8 rules.l2_and_l3_bounce 

.PHONY: all clean

all: ${TARGET}
	${MKDIR} ${REGRULES}/hxecache/
	${CP} ${TARGET} ${REGRULES}/hxecache/ 

clean:
	${RM} -rf ${REGRULES}/hxecache/* 
