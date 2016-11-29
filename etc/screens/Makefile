include ../../htx.mk

TARGET= AD_scn     \
        ARTD_scn    \
        RD_scn    \
        ddhelp_scn    \
        htx_scn    \
        mmenu_scn.S \
        AD_scn.a   \
        ARTD_scn.a  \
        RD_scn.a  \
        ddhelp_scn.a  \
        htx_scn.a  \
        mmenu_scn.a \
        AHD_scn    \
        COE_scn     \
        TD_scn    \
        dst_scn   \
        mmhelp_scn \
        AHD_scn.a  \
        COE_scn.a   \
        TD_scn.a  \
        dst_scn.a  \
        mmenu_scn  \
        mmhelp_scn.a \
        config_net  \
        config_net.a \
        htxscreen_mmenu_scn  \
        htxscreen_mmenu_scn.a \
        mdt_list  \
        mdt_list.a \
        ddsrch_scn \
        ddsrch_scn.a 
 

.PHONY: all clean

all: ${TARGET}
	@echo "making dir - "${SCREENS}
	${MKDIR} ${SCREENS}
	${CP} ${TARGET} ${SCREENS}

clean:
	${RM} -f ${SCREENS}/*
