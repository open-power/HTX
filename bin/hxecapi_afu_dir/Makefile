include ../../htx.mk

TARGET= hxecapi_afu_dir

LDFLAGS +=

OBJ_SUFF=.o
hxecapi_afu_dir_OBJECTS = memcopy_afu_directed.o get_rule.o memcpy_afu.o

hxecapi__afu_dir_SRCS = $(patsubst %.o, %.c, $(hxecapi_afu_directed_OBJECTS))

all: hxecapi_afu_dir

LIBS = -lcxl -lpthread -lhtx64
 
.PHONY: all clean

hxecapi_afu_dir: ${hxecapi_afu_dir_OBJECTS}
	${CC} ${LDFLAGS} ${LIBPATH} ${hxecapi_OBJECTS} ${LIBS} -o hxecapi_afu_dir 
	${CP} hxecapi_afu_dir ${SHIPBIN}

%.o: %.c
	${CC} ${CFLAGS} ${INCLUDES} -c $< -o $@

clean:
	${RM} -f *.o ${TARGET}
	${RM} -f ${SHIPBIN}/hxecapi_afu_dir
