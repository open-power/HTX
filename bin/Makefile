include ../htx.mk

SUBDIRS= hxssup hxsmsg hxstats htxd hxesamp show_syscfg  hxetlbie \
	hxestorage hxecom hxediag hxedapl hxemem64 hxetape hxesctu hxefpu64 hxecd  \
	hxecache hxerng bufdisp hxeasy hxefabricbus create_eq_cfg create_eq_mdt

ifeq ($(HTX_RELEASE), $(filter ${HTX_RELEASE},"htxrhel72le" "htxrhel7" "htxubuntu"))
	SUBDIRS+=hxecorsa
endif

SUBDIRS_CLEAN = $(patsubst %,%.clean,$(SUBDIRS))

.PHONY: all clean ${SUBDIRS} ${SUBDIRS_CLEAN}

all: ${SUBDIRS}
	${MKDIR} ${SHIPBIN}

${SUBDIRS}:
	@echo "making dir - "${SHIPBIN}
	${MKDIR} ${SHIPBIN}
	$(MAKE) -C $@

clean: ${SUBDIRS_CLEAN}

${SUBDIRS_CLEAN}:
	@$(MAKE) -C $(@:.clean=) clean

%.clean: %
	@$(MAKE) -C $< clean
