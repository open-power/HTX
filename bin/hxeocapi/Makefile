include ../../htx.mk

TARGET=hxeocapi

LDFLAGS +=

OBJ_SUFF=.o

CFLAGS += -O2 -g -D_GNU_SOURCE -D__HTX_LINUX__
CFLAGS += -I/usr/local/include

LIBS += -lhtx64 -lpthread -lrt -locxl -Wl,-rpath=/usr/local/lib64/ 

# Object files to build, of the form objname${OBJ_SUFF}
OBJECTS = get_rule${OBJ_SUFF} hxeocapi${OBJ_SUFF} memcpy${OBJ_SUFF} 

SRCS = $(patsubst %.o, %.c, $(OBJECTS))

.PHONY: all clean

all: ${OBJECTS}
	${CC} ${LDFLAGS} ${LIBPATH} ${OBJECTS} ${LIBS} -o ${TARGET}
	${CP} ${TARGET} ${SHIPBIN}/

%.o: %.c
	${CC} ${CFLAGS} ${INCLUDES} -c $< -o $@

#%.o: %.s
#       ${AS} ${CFLAGS} ${INCLUDES} -c $< -o $@

clean:
	${RM} -f *.o ${TARGET} ${SHIPBIN}/${TARGET}

