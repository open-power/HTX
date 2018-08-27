include ../../htx.mk

TARGET=hxestorage

LDFLAGS +=

OBJ_SUFF=.o

CFLAGS += -O2 -D_GNU_SOURCE -D__HTX_LINUX__ -D_target_ppc -D_FILE_OFFSET_BITS=64

LIBS += -lhtx64 -lpthread -lrt

# Object files to build, of the form objname${OBJ_SUFF}
OBJECTS = \
                hxestorage_rf${OBJ_SUFF} \
                io_oper${OBJ_SUFF} \
                hxestorage_utils${OBJ_SUFF} \
                hxestorage${OBJ_SUFF} \
                ioctl_linux${OBJ_SUFF} \
                analyze_miscompare${OBJ_SUFF} 



SRCS = $(patsubst %.o, %.c, $(OBJECTS))

.PHONY: all clean

all: ${OBJECTS}
	${CC} ${LDFLAGS} ${LIBPATH} ${OBJECTS} ${LIBS} -o ${TARGET}
	${CP} ${TARGET} ${SHIPBIN}/

%.o: %.c
	${CC} ${CFLAGS} ${INCLUDES} -c $< -o $@

#%.o: %.s
#	${AS} ${CFLAGS} ${INCLUDES} -c $< -o $@

clean:
	${RM} -f *.o ${TARGET} ${SHIPBIN}/${TARGET}
