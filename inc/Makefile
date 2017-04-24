include ..//htx.mk

SUBDIRS= sys iomm
SUBDIRS_CLEAN = $(patsubst %,%.clean,$(SUBDIRS))

TARGET= bufupd.h \
                        cfg04.h \
                        cfgclib.h \
                        cfgclibdef.h \
                        cfgcom.h \
                        cfgpest.h \
                        htx_local.h \
                        htxlibdef.h \
                        hxiconv.h \
                        hxihtx.h \
                        hxiipc.h \
                        sevcodes.h \
                        scr_info.h \
                        hxiipc64.h \
                        hxihtx64.h \
                        scr_info64.h \
                        htxsyscfg64.h \
                        hxerng.h \
                        getrule.h \
			hxfcpp_wrap.H \
                        expirekey.h \
			xscom.h \
                        nx_corsa.h \
			fpu_common_struct_prot.h \
			instr_form_BE.h \
			instr_form_LE.h
 
.PHONY: all ${SUBDIRS} clean

all: ${SUBDIRS}
	${MKDIR} ${EXPINC}
	${CP} ${TARGET} ${EXPINC}

${SUBDIRS}:
	${MKDIR} ${EXPINC}
	$(MAKE) -C $@

.PHONY: ${SUBDIRS_CLEAN} clean_local

clean: ${SUBDIRS_CLEAN}

clean_local:
	${RM} -rf ${EXPINC}/${TARGET} 

${SUBDIRS_CLEAN}:
	@$(MAKE) -C $(@:.clean=) clean

%.clean: %
	@$(MAKE) -C $< clean
