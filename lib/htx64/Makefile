include ../../htx.mk

TARGET= libhtx64.a

LDFLAGS +=

OBJ_SUFF=.o

OBJECTS =hxfupdate${OBJ_SUFF} \
		 hxfpat${OBJ_SUFF} \
		 hxfmsg${OBJ_SUFF} \
         hxfcbuf${OBJ_SUFF} \
         linux${OBJ_SUFF} \
         hxf_sig_safe${OBJ_SUFF} \
	     htxsyscfg${OBJ_SUFF} \
	     xscom${OBJ_SUFF} \
	     misc${OBJ_SUFF} \
	     htxmp_new${OBJ_SUFF} \
	     hxfsbuf${OBJ_SUFF}

SRCS = $(patsubst %.o, %.c, $(OBJECTS))

#CFLAGS +=
CFLAGS += -D__64_LINUX__ -D_GNU_SOURCE -DKERNEL_2_6 -D_REENTRANT -D__HTX_LINUX__ -O2 -DGLIBC_GR -DLOG_TO_PHY_NEEDED -DMFPVR_ALLOWED  -DGLIBC_GR


.PHONY: all clean

all: ${OBJECTS}
	    ${AR} ${ARFLAGS} ${TARGET} ${OBJECTS}
		    ${CP} ${TARGET} ${EXPLIB}

%.o: %.c
	    ${CC} ${CFLAGS} ${INCLUDES} -c $< -o $@

clean:
	    ${RM} -rf *.o ${TARGET} ${EXPLIB}/${TARGET}

