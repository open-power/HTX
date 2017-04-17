include ../../../htx.mk

TARGET= \
	maxmem  \
             maxmem_rim  \
             hxemem64.readme  \
             mem_16m_hwpf \
             mem_16m_hwpf_old \
             memmfg.p6 \
             memmfg \
             memmfg.p6.nodelay \
             memmfg.nodelay \
             memconfig.txt \
             default.mem.eq.4k \
             default.mem.eq.64k \
             rules.dl1_reloads_from_caches \
             rules.dl1_reloads_from_caches_old \
             rules.max_cache_hit \
             rules.max_cache_hit_old \
             L4 \
             rules.max_cache_miss \
             rules.max_cache_miss_old \
             rules.comm_db_pk \
             rules.comm_db_pk_old \
             rules.web_server \
             rules.web_server_old \
             rules.app_server \
             rules.app_server_old \
             rules.hpc_comp_node \
             rules.hpc_comp_node_old \
             rules.MBA \
      		 mem.eq.50 \
	    	 mem.eq.90 \
             mem.coral_pass1 \
             default \
             default.char \
             default.less \
             default.eq \
             default.mfg \
             default.mfg.nodelay


.PHONY: all clean

all: ${TARGET}
	${MKDIR} ${REGRULES}/hxemem64/
	${CP} ${TARGET} ${REGRULES}/hxemem64/ 

clean:
	${RM} -rf ${REGRULES}/hxemem64/*
