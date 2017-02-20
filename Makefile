include htx.mk

SUBDIRS= inc lib bin rules etc mdt cleanup pattern setup runsetup \
	runcleanup Documentation equaliser_cfgs
SUBDIRS_CLEAN = $(patsubst %,%.clean,$(SUBDIRS))

TARGET= .htx_profile \
        .htxrc \
        .bash_profile \
        .bashrc \
        htx_eq.cfg \
        htx_diag.config \
	run_htx_cmdline.sh \
        hxsscripts

.PHONY: all ${SUBDIRS} deb

default: all

lib: inc

bin: lib

${SUBDIRS}:
	$(MAKE) -C $@

all: ${SUBDIRS}
	@echo "making dir - "${SHIPTOPDIR}
	${MKDIR} ${SHIPTOPDIR}
	${CP} ${TARGET} ${SHIPTOPDIR}

.PHONY: clean ${SUBDIRS_CLEAN} clean_local

clean: ${SUBDIRS_CLEAN} clean_local
	@echo "Removing dir - "${SHIPDIR}
	${RM} -rf ${SHIPDIR}
	@echo "Removing dir - "${EXPORT}
	${RM} -rf ${EXPORT}

${SUBDIRS_CLEAN}:
	@$(MAKE) -C $(@:.clean=) clean

%.clean: %
	@$(MAKE) -C $< clean

deb:
	@echo "Making HTX Debian package..."
	cp -r $(PACKAGINGDIR)/ubuntu/* $(SHIPDIR)/
	dpkg-deb -b $(SHIPDIR)  $(TOPDIR)/htxubuntu.deb
