include ../../../htx.mk

TARGET= \
	rules.default \
	rules.64 \
        default.cpu.eq \
        rules.branch \
        rules.larx_stcx \
        rules.shl_lhl \
        rules.prefetch_n_sync \
        rules.sao \
        rules.thread_priority \
        rules.unalign \
        rules.vsx.vmx \
        rules.unalign_shl_lhl \
        rules.comm_db_pk \
        cpu.eq_th_trans_switch \
        rules.business_int_db_pk \
        rules.28cp0 \
        rules.215cp0 \
        rules.915cp0 \
        default.p6 \
        default.p7 \
        default.p8 \
        default.p9 \
        hxecpu.readme

.PHONY: all clean

all: ${TARGET}
	${MKDIR} ${REGRULES}/hxecpu/
	${CP} ${TARGET} ${REGRULES}/hxecpu/ 

clean:
	${RM} -rf ${REGRULES}/hxecpu/* 
