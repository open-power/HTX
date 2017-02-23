include ../../htx.mk

TARGET= hxecapi

LDFLAGS +=

OBJ_SUFF=.o
hxecapi_OBJECTS = memcopy.o get_rule.o

hxecapi_SRCS = $(patsubst %.o, %.c, $(hxecapi_OBJECTS))

all: hxecapi

CFLAGS += -D__RELGR2600__
LIBS = -lcxl -lpthread -lhtx64
 
.PHONY: all clean

hxecapi: ${hxecapi_OBJECTS}
	${CC} ${LDFLAGS} ${LIBPATH} ${hxecapi_OBJECTS} ${LIBS} -o hxecapi 
	${CP} hxecapi ${SHIPBIN}

%.o: %.c
	${CC} ${CFLAGS} ${INCLUDES} -c $< -o $@

clean:
	${RM} -f *.o ${TARGET}
	${RM} -f ${SHIPBIN}/hxecapi
