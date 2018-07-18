include ../../htx.mk

SUBDIRS= hxecd hxetape hxeasy hxetlbie hxecorsa \
        hxemem64 bootme hxecapi hxecapi_afu_dir hxecom hxediag hxerng hxecpu hxefpu64 hxesctu \
        hxecache hxefabricbus hxesamp hxestorage 
SUBDIRS_CLEAN = $(patsubst %,%.clean,$(SUBDIRS))

.PHONY: all ${SUBDIRS} clean ${SUBDIRS_CLEAN}

all: ${SUBDIRS}
	@echo "making dir - ${REGRULES}"
	${MKDIR} ${REGRULES}

clean: ${SUBDIRS_CLEAN}

${SUBDIRS}:
	$(MAKE) -C $@

${SUBDIRS_CLEAN}:
	@$(MAKE) -C $(@:.clean=) clean

%.clean: %
	@$(MAKE) -C $< clean
