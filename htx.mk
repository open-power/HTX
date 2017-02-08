# Please read README.md for info about how to build HTX.

#Set HTX_RLEASE to the distro you want to build for
HTX_RELEASE="htxubuntu"
ifeq ($(HTX_RELEASE), $(filter ${HTX_RELEASE},"htxubuntu" "htxsles12" "htxrhel72le" "htxfedorale"))
ARCH=ppc64le
else
ARCH=ppc64
endif
MKDIR=/bin/mkdir -p
AS?=/usr/bin/as
CC?=/usr/bin/gcc -m64
CXX?=/usr/bin/g++ -m64
RM=/bin/rm
CP=/bin/cp
AR?=/usr/bin/ar
LD?=/usr/bin/ld
LDFLAGS=
CFLAGS=-D__HTX_LINUX__ -DTRUE=1 -DFALSE=0 -D__64BIT__
ifeq ($(HTX_RELEASE), $(filter ${HTX_RELEASE},"htxubuntu" "htxsles12" "htxrhel72le" "htxfedorale"))
	CFLAGS+= -D__HTX_LE__
endif
#Set the TOPDIR before starting build 
TOPDIR=$(dir $(abspath $(lastword $(MAKEFILE_LIST))))
SHIPDIR=${TOPDIR}/install/${ARCH}/
HTXOBJDIR=${TOPDIR}/obj/
SHIPTOPDIR=${SHIPDIR}/usr/lpp/htx/
SHIPDOCDIR=${SHIPTOPDIR}/Documentation/
PACKAGINGDIR=${TOPDIR}/packaging/
EXPORT=${TOPDIR}/export/${ARCH}/
EXPINC=${EXPORT}/include/
EXPLIB=${EXPORT}/lib/
INCLUDES=-I./ -I/usr/include/ -I${EXPINC}
LIBPATH=-L${EXPLIB}/
SHIPBIN=${SHIPTOPDIR}/bin/
RUNCLEANUP=${SHIPTOPDIR}/runcleanup/
RUNSETUP=${SHIPTOPDIR}/runsetup/
CLEANUP=${SHIPTOPDIR}/cleanup
PATTERN=${SHIPTOPDIR}/pattern
CFGPATH=${SHIPTOPDIR}/equaliser_cfgs/
REGRULES=${SHIPTOPDIR}/rules/reg/
SETUP=${SHIPTOPDIR}/setup/
MDT=${SHIPTOPDIR}/mdt/
ETC=${SHIPTOPDIR}/etc/
SCRIPTS=${ETC}/scripts/
SCRIPTS_STX=${ETC}/scripts_stx/
SCREENS=${ETC}/screens/
SCREENS_STX=${ETC}/screens_stx/

