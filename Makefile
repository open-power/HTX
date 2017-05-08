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
	touch /${SHIPTOPDIR}/.htx_build_done

.PHONY: clean ${SUBDIRS_CLEAN} clean_local

clean: ${SUBDIRS_CLEAN} clean_local
	@echo "Removing dir - "${SHIPDIR}
	${RM} -rf ${SHIPDIR}
	@echo "Removing dir - "${EXPORT}
	${RM} -rf ${EXPORT}
	${RM} -rf htx_package.tar.gz
	${RM} -rf htxubuntu.deb
	@if [ -e /${SHIPTOPDIR}/.htx_build_done ]; \
	then \
		${RM} /${SHIPTOPDIR}/.htx_build_done ; \
	fi

${SUBDIRS_CLEAN}:
	@$(MAKE) -C $(@:.clean=) clean

%.clean: %
	@$(MAKE) -C $< clean

deb:
	@echo "Making HTX Debian package..."
	cp -r $(PACKAGINGDIR)/ubuntu/* $(SHIPDIR)/
	dpkg-deb -b $(SHIPDIR)  $(TOPDIR)/htxubuntu.deb

tar:    
	@if [ ! -f /${SHIPTOPDIR}/.htx_build_done ] ; \
        then \
                echo -e "HTX is not built for tar package creation, \ndo 'make all' and retry, exiting...";  \
		exit 1; \
        fi;
	
	@echo "Making TAR package..."
	@echo "making dir - "${SHIPTOPDIR}/htx_binaries
	${MKDIR} ${SHIPTOPDIR}/htx_binaries
	${CP} -r ${SHIPDIR}/usr/bin ${SHIPTOPDIR}/htx_binaries/
	${CP} -r ${SHIPDIR}/usr/sbin ${SHIPTOPDIR}/htx_binaries/
	echo -ne "Compiledon-" > ${SHIPTOPDIR}/etc/version
	date '+%d/%m/%Y %H:%M:%S'  >> ${SHIPTOPDIR}/etc/version
	${RM} -rf htx_package
	${MKDIR} htx_package
	tar -cvzf htx_package/htx.tar.gz -C ${SHIPDIR}/usr/lpp htx > /dev/null
	${CP} $(PACKAGINGDIR)/installer.sh htx_package
	${CP} $(PACKAGINGDIR)/uninstaller.sh htx_package
	@if [ -f /etc/redhat-release ] ; \
	then \
	echo "Compiledon:RedHat" > htx_package/os_build ; \
	elif [ -f /etc/SuSE-release ] ; \
	then \
	echo "Compiledon:SUSE" > htx_package/os_build ; \
	elif [ -f /etc/debian_version ] ; \
	then \
	echo "Compiledon:Ubuntu" > htx_package/os_build ; \
	fi;
	echo -e "\nBuilt-on-Distro: " >> htx_package/os_build ;
	cat /etc/*-release >> htx_package/os_build ;
	echo -e "\nGLIBC Version: " >> htx_package/os_build ;
	ldd --version | grep ldd >> htx_package/os_build ;
	echo -e "\ngcc version: " >> htx_package/os_build ;
	gcc --version | grep gcc >> htx_package/os_build ;
	echo -e "\nkernel version: " >> htx_package/os_build ;
	uname -r >> htx_package/os_build ;
	tar -cvzf htx_package.tar.gz htx_package > /dev/null
	${RM} -rf ${SHIPTOPDIR}/htx_binaries/
	${RM} -rf htx_package
	@if [ -f ./htx_package.tar.gz ] ; \
	then \
		echo -e "HTX package htx_package.tar.gz is generated successfully..."; \
	else \
		echo -e "HTX package generation failed.. exiting...";  \
	fi;
