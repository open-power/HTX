include ../../../htx.mk

TARGET= \
	default.p6 \
        default.p7 \
        default.p8 \
        default.p9 \
        default.bfp \
        default.dfp \
        default.vsx \
        bypass_mix1 \
        bypass_mix2 \
        data_security \
        fpscr_bfu \
        fpscr_vsx \
        vsx_rounding \
        default.fpu.eq \
        vscr \
        stress \
        p1_lock \
        xs_dfp \
        rules.interleaving \
        rules.sao \
        rules.vsx.vmx \
        rules.multi \
        rules.hpc_comp_node \
	fpu_cpu_combo.p8 \
	fpu_cpu_combo.p9 \
        hxefpu64.readme

.PHONY: all clean

all: ${TARGET}
	${MKDIR} ${REGRULES}/hxefpu64/
	${CP} ${TARGET} ${REGRULES}/hxefpu64/ 

clean:
	${RM} -rf ${REGRULES}/hxefpu64/* 
